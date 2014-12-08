#include "neopixel.h" 

#define SYSTEM_CORE_CLOCK                   180000000
#define DATA_SPEED                          800000  
#define BITS_PER_INTERRUPT                  24
#define PRESCALER                           SYSTEM_CORE_CLOCK / (25 * DATA_SPEED)
#define COUNTER_REG_SIZE                    16

#define PERIOD_EVENT_NUM                    15
#define T1H_EVENT_NUM                       14
#define T0H_EVENT_NUM                       13
#define CHAN_A_OUTPUT_BUFFER_EVENT_NUM      12
#define CHAN_A_AUX_BUFFER_EVENT_NUM         11
#define COMPLETE_FRAME_EVENT                10

#define CHAN_B_PERIOD_EVENT_NUM             9
#define CHAN_B_T1H_EVENT_NUM                8
#define CHAN_B_T0H_EVENT_NUM                7
#define CHAN_B_OUTPUT_BUFFER_EVENT_NUM      6
#define CHAN_B_AUX_BUFFER_EVENT_NUM         5
#define CHAN_B_COMPLETE_FRAME_EVENT         4

neopixel_animation_status_t channel_a_animation;
neopixel_animation_status_t channel_b_animation;

const neopixel_sct_status_t sct_animation_channels[MAX_SCT_CHANNELS] = {
  {
    .pin = E_G4,
    .sctRegOffset = 1,
    .animationStatus = &channel_a_animation,
    .periodEventNum = PERIOD_EVENT_NUM,
    .t1hEventNum = T1H_EVENT_NUM,
    .t0hEventNum = T0H_EVENT_NUM,
    .sctOutputBuffer = CHAN_A_OUTPUT_BUFFER_EVENT_NUM,
    .sctAuxBuffer = CHAN_A_AUX_BUFFER_EVENT_NUM,
    .completeFrameEvent = COMPLETE_FRAME_EVENT,
    .sctOutputChannel = 8,
    .sctAuxChannel = 1,
  },
  {
  .pin = E_G5,
  .sctRegOffset = 0,
  .animationStatus = &channel_b_animation,
  .t1hEventNum = CHAN_B_T1H_EVENT_NUM,
  .t0hEventNum = CHAN_B_T0H_EVENT_NUM,
  .sctOutputBuffer = CHAN_B_OUTPUT_BUFFER_EVENT_NUM,
  .sctAuxBuffer = CHAN_B_AUX_BUFFER_EVENT_NUM,
  .completeFrameEvent = CHAN_B_COMPLETE_FRAME_EVENT,
  .sctOutputChannel = 5,
  .sctAuxChannel = 2,
  }
};

void beginAnimationAtCurrentFrame();
void continueAnimation();
bool updateChannelAnimation(neopixel_sct_status_t channel);
void emit_complete(uint8_t channel);
void animation_a_complete(tm_event *event);
void animation_b_complete(tm_event *event);

tm_event animation_a_complete_event = TM_EVENT_INIT(animation_a_complete); 
tm_event animation_b_complete_event = TM_EVENT_INIT(animation_b_complete); 

void LEDDRIVER_open (neopixel_sct_status_t sct_channel)
{
  uint32_t clocksPerBit;

  /* Halt H timer, and configure counting mode and prescaler.
   */
  uint32_t counterConfig = 0
      | (0 << SCT_CTRL_DOWN_Pos)
      | (0 << SCT_CTRL_STOP_Pos)
      | (1 << SCT_CTRL_HALT_Pos)      /* HALT counter */
      | (1 << SCT_CTRL_CLRCTR_Pos)
      | (0 << SCT_CTRL_BIDIR_Pos)
      | (((PRESCALER - 1) << SCT_CTRL_PRE_Pos) & SCT_CTRL_PRE_Msk);
      ;

  // Set the control register depending on which counter this channel is using
  // The high counter is 16 bits off of the low counter registers
  LPC_SCT->CTRL_U = (counterConfig << (sct_channel.sctRegOffset * COUNTER_REG_SIZE));

  /* Start state */
  (&LPC_SCT->STATE_L)[sct_channel.sctRegOffset] = BITS_PER_INTERRUPT;

  /* Counter LIMIT */
  (&LPC_SCT->LIMIT_L)[sct_channel.sctRegOffset] = (1u << sct_channel.periodEventNum); /* Event 15 */

  /* Configure the match registers */
  clocksPerBit = SYSTEM_CORE_CLOCK / (PRESCALER * DATA_SPEED);
  (&LPC_SCT->MATCHREL_L[0])[sct_channel.sctRegOffset * 2 * COUNTER_REG_SIZE] = clocksPerBit - 1;
  (&LPC_SCT->MATCHREL_L[1])[sct_channel.sctRegOffset * 2 * COUNTER_REG_SIZE] = 8 - 1;
  (&LPC_SCT->MATCHREL_L[2])[sct_channel.sctRegOffset * 2 * COUNTER_REG_SIZE] = 16 - 1;

  /* Configure events */
  LPC_SCT->EVENT[sct_channel.periodEventNum].CTRL = 0
      | (0 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH0_H */
      | (sct_channel.sctRegOffset << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      | (0 << SCT_EVx_CTRL_STATELD_Pos)   /* Add value to STATE */
      | (31 << SCT_EVx_CTRL_STATEV_Pos)   /* Add 31 (i.e subtract 1) */
      ;
  LPC_SCT->EVENT[sct_channel.t1hEventNum].CTRL = 0
      | (2 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH2_H */
      | (sct_channel.sctRegOffset << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      ;
  LPC_SCT->EVENT[sct_channel.t0hEventNum].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (sct_channel.sctRegOffset << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      ;

  // POTENTIAL PROBLEM
  // LPC_SCT->RES = 0;

  if (sct_channel.animationStatus->animation.frames != NULL) {

    LPC_SCT->OUTPUT &= ~(0 
      | (1u << sct_channel.sctOutputChannel)
    );
    LPC_SCT->OUTPUT |= (1u << sct_channel.sctAuxChannel);
    
    LPC_SCT->EVENT[sct_channel.sctOutputBuffer].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (sct_channel.sctRegOffset << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_OUTSEL_Pos)    /* Use OUTPUT for I/O condition */
      | (sct_channel.sctAuxChannel << SCT_EVx_CTRL_IOSEL_Pos)    /* Use AUX signal */
      | (0 << SCT_EVx_CTRL_IOCOND_Pos)    /* AUX = 0 */
      | (3 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH AND I/O */
      ;
    LPC_SCT->EVENT[sct_channel.sctAuxBuffer].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (sct_channel.sctRegOffset << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_OUTSEL_Pos)    /* Use OUTPUT for I/O condition */
      | (sct_channel.sctAuxChannel << SCT_EVx_CTRL_IOSEL_Pos)    /* Use AUX signal */
      | (3 << SCT_EVx_CTRL_IOCOND_Pos)    /* AUX = 1 */
      | (3 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH AND I/O */
      ;

    LPC_SCT->EVENT[sct_channel.sctOutputBuffer].STATE = 0;
    LPC_SCT->EVENT[sct_channel.sctAuxBuffer].STATE = 0; 

    LPC_SCT->OUT[sct_channel.sctAuxChannel].SET = 0
        | (1u << sct_channel.completeFrameEvent)                        /* Complete Event toggles the AUX signal */
        ;
    LPC_SCT->OUT[sct_channel.sctAuxChannel].CLR = 0
        | (1u << sct_channel.completeFrameEvent)                        /* Complete Event toggles the AUX signal */
        ;
    LPC_SCT->OUT[sct_channel.sctOutputChannel].SET = 0
        | (1u << sct_channel.periodEventNum)                        /* Event 15 sets the DATA signal */
        | (1u << sct_channel.sctOutputBuffer)                        /* Event 12 sets the DATA signal */
        | (1u << sct_channel.sctAuxBuffer)                        /* Event 11 sets the DATA signal */
        ;
    LPC_SCT->OUT[sct_channel.sctOutputChannel].CLR = 0
        | (1u << sct_channel.t1hEventNum)                        /* Event 14 clears the DATA signal */
        | (1u << sct_channel.t0hEventNum)                        /* Event 13 clears the DATA signal */
        | (1u << sct_channel.completeFrameEvent)                        /* Complete Event clears the DATA signal */
        ;

    LPC_SCT->RES |= 0
          | (0 << 2 * sct_channel.sctOutputChannel)            /* DATA signal doesn't change */
          | (3 << 2 * sct_channel.sctAuxChannel) 
          ;
  }

  LPC_SCT->EVENT[sct_channel.completeFrameEvent].CTRL = 0
      | (2 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH2_H */
      | (sct_channel.sctRegOffset << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      | (1 << SCT_EVx_CTRL_STATELD_Pos)   /* Set STATE to a value */
      | (BITS_PER_INTERRUPT << SCT_EVx_CTRL_STATEV_Pos)   /* Set to 8 */
      ;
  LPC_SCT->EVENT[sct_channel.periodEventNum].STATE = 0xFFFFFFFF;
  LPC_SCT->EVENT[sct_channel.t1hEventNum].STATE = 0x00FFFFFE;  /* All data bit states except state 0 */
  LPC_SCT->EVENT[sct_channel.t0hEventNum].STATE = 0x00FFFFFF;  /* All data bit states */
  LPC_SCT->EVENT[sct_channel.completeFrameEvent].STATE = 0x00000001; /* Only in state 0 */

  /* Default is to halt the block transfer after the next frame */
  LEDDRIVER_haltAfterFrame(1, sct_channel);           /* Complete Event halts the transfer */

  // Clear pending interrupts on period completion
  LPC_SCT->EVFLAG = (1u << sct_channel.completeFrameEvent);

  /* Configure interrupt events */
  LPC_SCT->EVEN |= (1u << sct_channel.completeFrameEvent);            /* Complete Event */
}



/* Simple function to write to a transmit buffer. */
void LEDDRIVER_writeNextRGBValue (neopixel_sct_status_t sct_channel)
{
  
  // Make sure this channel actually has animations
  if (sct_channel.animationStatus->animation.frames != NULL) {
    // Get the current rgb value
    uint32_t rgb = (sct_channel.animationStatus->animation.frames[sct_channel.animationStatus->framesSent][sct_channel.animationStatus->bytesPending] << 16
                 | sct_channel.animationStatus->animation.frames[sct_channel.animationStatus->framesSent][sct_channel.animationStatus->bytesPending + 1] << 8
                 | sct_channel.animationStatus->animation.frames[sct_channel.animationStatus->framesSent][sct_channel.animationStatus->bytesPending + 2]);
    // Set the rgb value to the appropriate state
    if (LPC_SCT->OUTPUT & (1u << sct_channel.sctAuxChannel)) {
      LPC_SCT->EVENT[sct_channel.sctOutputBuffer].STATE = (rgb & 0xFFFFFF);
    }
    else {
      LPC_SCT->EVENT[sct_channel.sctAuxBuffer].STATE = (rgb & 0xFFFFFF);
    }
  }

  sct_channel.animationStatus->bytesPending += 3;
}

/* Activate or deactivate HALT after next frame. */
void LEDDRIVER_haltAfterFrame (int on, neopixel_sct_status_t sct_channel)
{
  if (on) {
    (&LPC_SCT->HALT_L)[sct_channel.sctRegOffset] = (1u << sct_channel.completeFrameEvent);
  }
  else {
    (&LPC_SCT->HALT_L)[sct_channel.sctRegOffset] = 0;
  }
}

/* Start a block transmission */
void LEDDRIVER_start (neopixel_sct_status_t sct_channel)
{
  /* TODO: Check whether timer is really in HALT mode */

  /* Set reset time */
  LPC_SCT->COUNT_U = -(&LPC_SCT->MATCHREL_L[0])[sct_channel.sctRegOffset * 2 * COUNTER_REG_SIZE] * 50;     /* TODO: Modify this to guarantee 50 µs min in both modes! */

  /* Start state */
  (&LPC_SCT->STATE_L)[sct_channel.sctRegOffset] = BITS_PER_INTERRUPT;

  /* Start timer H */
  LPC_SCT->CTRL_U &= (~SCT_CTRL_H_HALT_H_Msk << (sct_channel.sctRegOffset * COUNTER_REG_SIZE));
}

void sct_neopixel_irq_handler (void)
{
  for (uint8_t i = 0; i < MAX_SCT_CHANNELS; i++) {

    if (LPC_SCT->EVFLAG & (1u << sct_animation_channels[i].completeFrameEvent)
      && sct_animation_channels[i].animationStatus->currentState == RUNNING) {

       // Unset IRQ flag
      LPC_SCT->EVFLAG = (1u << sct_animation_channels[i].completeFrameEvent);
      
      // Attempt to update their channel
      if (!updateChannelAnimation((sct_animation_channels[i]))) {

        // Trigger the callback
        if (i == 0) {
          TM_DEBUG("TA");
          // tm_event_trigger(&animation_a_complete_event);
        } 
        else if (i == 1) {
          TM_DEBUG("TB");
          // tm_event_trigger(&animation_b_complete_event);
        }
      }
    }
  }
}

// Updates the SCT with the next byte of an animation
// Returns the number of bytes updated (0 or 1)
bool updateChannelAnimation(neopixel_sct_status_t sct_channel) {

  bool byteSent = false;

  sct_channel.animationStatus->bytesSent+=3;

  // If we have not yet sent all of our frames
  if (sct_channel.animationStatus->framesSent < sct_channel.animationStatus->animation.numFrames) {

    // If we have not yet sent all of our bytes in the current frame
    if (sct_channel.animationStatus->bytesSent <= sct_channel.animationStatus->animation.frameLength) {

      // Send the next byte
      LEDDRIVER_writeNextRGBValue(sct_channel); 

      byteSent = true;

      uint32_t bytesRemaining = sct_channel.animationStatus->animation.frameLength - sct_channel.animationStatus->bytesSent;

      // If we only have one byte left (but it's double buffered, so the 2nd last byte is current being sent)
      if (bytesRemaining == 3) {

        // We're going to halt
        LEDDRIVER_haltAfterFrame(1, sct_channel);

      }

      // If we have sent all of the bytes in this frame (bytesSent is pre-)
      else if (bytesRemaining == 0) {

        // Move onto the next
        sct_channel.animationStatus->framesSent++;
        sct_channel.animationStatus->bytesSent = 0;
        sct_channel.animationStatus->bytesPending = 0;
        // If we have now sent all of them
        if (sct_channel.animationStatus->framesSent == sct_channel.animationStatus->animation.numFrames) {
          sct_channel.animationStatus->currentState = EMITTING;
          // Trigger the end
          byteSent = false;
        }

        // If not all frames have been sent
        else {
          // Continue with the next frame
          continueAnimation(sct_channel);

          byteSent = true;
        }
      }
    }
  }

  return byteSent;
}

void neopixel_reset_animation(bool force) {

  // POTENTIAL PROBLEM
  // Disable the SCT IRQ
  // NVIC_DisableIRQ(SCT_IRQn);

  // POTENTIAL PROBLEM
  // set the SCT state back to inactive
  // hw_sct_status = SCT_INACTIVE;

  // POTENTIAL PROBLEM
  // really clear the SCT: ( 1 << 5 ) is an LPC18xx.h include workaround
  // LPC_RGU->RESET_CTRL1 = ( 1 << 5 );

  // Make sure the Lua state exists
  lua_State* L = tm_lua_state;
  if (!L) return;

  for (int i = 0; i < MAX_SCT_CHANNELS; i++) {

    // This one finished!
    TM_DEBUG("Testing... %d %d %d", i, sct_animation_channels[i].animationStatus->animation.numFrames, sct_animation_channels[i].animationStatus->framesSent);
    if (sct_animation_channels[i].animationStatus->currentState == EMITTING || force) {

      (&LPC_SCT->HALT_L)[sct_animation_channels[i].sctRegOffset] = sct_animation_channels[i].completeFrameEvent;


      // Unreference our buffer so it can be garbage collected
      luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, sct_animation_channels[i].animationStatus->animation.frameRef);

      // Free our animation buffers and struct memory
      free(sct_animation_channels[i].animationStatus->animation.frames);
      
      sct_animation_channels[i].animationStatus->animation.frames = NULL;
      sct_animation_channels[i].animationStatus->animation.frameLength = 0;
      sct_animation_channels[i].animationStatus->animation.numFrames = 0;
      sct_animation_channels[i].animationStatus->animation.frameRef = -1;
      sct_animation_channels[i].animationStatus->currentState = IDLE;
    }

    if (i == 0) {
      // Unreference the event
      tm_event_unref(&animation_a_complete_event);

    }
    else {
      // Unreference the event
      tm_event_unref(&animation_b_complete_event);
    }
  }
}

void animation_a_complete(tm_event *event) {
  (void) event;
  // Reset all of our variables
  TM_DEBUG("trig a");
  neopixel_reset_animation(false);
  // Emit the event
  emit_complete(1);
}

void animation_b_complete(tm_event *event) {
  (void) event;
  // Reset all of our variables
  TM_DEBUG("trig b");
  neopixel_reset_animation(false);
  // Emit the event
  emit_complete(2);
}

void emit_complete(uint8_t channel) {
  lua_State* L = tm_lua_state;
  if (!L) return;
  // Push the _colony_emit helper function onto the stack
  lua_getglobal(L, "_colony_emit");
  // The process message identifier
  lua_pushstring(L, "neopixel_animation_complete");
  // Emit the completed animations
  lua_pushnumber(L, channel);
  // Call _colony_emit to run the JS callback
  tm_checked_call(L, 2);
}

void continueAnimation(neopixel_sct_status_t sct_channel) {

  // Todo: Make this extensible for more channels
  LEDDRIVER_writeNextRGBValue(sct_channel);

  // Don't halt
  LEDDRIVER_haltAfterFrame(0, sct_channel);

  // Set the flag to get an interrupt when byte transfer is complete
  LPC_SCT->EVEN |= (1u << sct_channel.completeFrameEvent);

   /* Set reset time */
  LEDDRIVER_start(sct_channel);
}

void beginAnimation(neopixel_sct_status_t sct_channel) {

  // Initialize the LEDDriver
  LEDDRIVER_open(sct_channel);

  // Fill the double buffer with the first two bytes
  for (int i = 0; i < MAX_SCT_CHANNELS; i++) {
    // Reset struct status vars
    sct_channel.animationStatus->bytesPending = 0;
    sct_channel.animationStatus->bytesSent = 0;
    // Write the first bytes to the output
    LEDDRIVER_writeNextRGBValue(sct_channel);
    // Toggle the AUX Output so the next write will go to Aux
    LPC_SCT->OUTPUT &= ~(1u << sct_channel.sctAuxChannel);
    // Write the next bytes to Aux
    LEDDRIVER_writeNextRGBValue(sct_channel);
  }

  // Allow SCT IRQs (which update the relevant data byte)
  NVIC_EnableIRQ(SCT_IRQn);

  // Do not halt after the first frame
  LEDDRIVER_haltAfterFrame(0, sct_channel);

  // Start the operation
  LEDDRIVER_start(sct_channel);

  sct_channel.animationStatus->currentState = RUNNING;
}

void setPinSCTFunc(uint8_t pin) {
  // Set up the pin as SCT out
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    PUP_DISABLE | PDN_DISABLE,
    g_APinDescription[pin].alternate_func);
}

int8_t writeAnimationBuffers(neopixel_animation_status_t channel_animation, uint8_t pin) {

  // POTENTIAL PROBLEM
  // really clear the SCT: ( 1 << 5 ) is an LPC18xx.h include workaround
  // LPC_RGU->RESET_CTRL1 = ( 1 << 5 );

  uint8_t animationReady = false;
  for (uint8_t i = 0; i < MAX_SCT_CHANNELS; i++) {
    if (pin == sct_animation_channels[i].pin) {

      // Set the animations
      memcpy(sct_animation_channels[i].animationStatus, &channel_animation, sizeof(neopixel_animation_status_t));

      // Set up the pin as SCT
      setPinSCTFunc(sct_animation_channels[i].pin);

      // Then start transmission 
      beginAnimation(sct_animation_channels[i]);

      // Hold the event queue open until we're done with this event
      if (i == 0) {
        tm_event_ref(&animation_a_complete_event);
      }
      else if (i == 1) {
        tm_event_ref(&animation_b_complete_event);
      }

      animationReady = true;
    }
  }
  if (!animationReady) {
    return -1;
  }

  return 0;
}
