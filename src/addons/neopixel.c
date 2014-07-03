#include "neopixel.h" 

#define DATA_SPEED                          800000  
#define BITS_PER_INTERRUPT                  24 // Used to be 24

#define PERIOD_EVENT_NUM                    15
#define T1H_EVENT_NUM                       14
#define T0H_EVENT_NUM                       13
#define CHAN_A_OUTPUT_BUFFER_EVENT_NUM      12
#define CHAN_A_AUX_BUFFER_EVENT_NUM         11
#define CHAN_B_OUTPUT_BUFFER_EVENT_NUM      10
#define CHAN_B_AUX_BUFFER_EVENT_NUM         9
#define CHAN_C_OUTPUT_BUFFER_EVENT_NUM      8
#define CHAN_C_AUX_BUFFER_EVENT_NUM         7
#define COMPLETE_FRAME_EVENT                6


volatile neopixel_sct_status_t channel_a = {
  .pin = E_G4,
  .animationStatus = NULL,
  .sctOuputBuffer = CHAN_A_OUTPUT_BUFFER_EVENT_NUM,
  .sctAuxBuffer = CHAN_A_AUX_BUFFER_EVENT_NUM,
  .sctOutputChannel = 8,
  .sctAuxChannel = 1,
};

volatile neopixel_sct_status_t channel_b = {
  .pin = E_G5,
  .animationStatus = NULL,
  .sctOuputBuffer = CHAN_B_OUTPUT_BUFFER_EVENT_NUM,
  .sctAuxBuffer = CHAN_B_AUX_BUFFER_EVENT_NUM,
  .sctOutputChannel = 5,
  .sctAuxChannel = 2,
};

volatile neopixel_sct_status_t channel_c = {
  .pin = E_G6,
  .animationStatus = NULL,
  .sctOuputBuffer = CHAN_C_OUTPUT_BUFFER_EVENT_NUM,
  .sctAuxBuffer = CHAN_C_AUX_BUFFER_EVENT_NUM,
  .sctOutputChannel = 10,
  .sctAuxChannel = 3,
};

volatile neopixel_sct_status_t *sct_animation_channels[] = {
  &channel_a,
  &channel_b,
  &channel_c,
};


#define AUX_A_OUTPUT                        channel_a.sctAuxChannel

#define CHAN_B_OUTPUT                       g_APinDescription[channel_b.pin].pwm_channel // 5 on TM-00-04
#define AUX_B_OUTPUT                        channel_b.sctAuxChannel

#define CHAN_C_OUTPUT                       g_APinDescription[channel_c.pin].pwm_channel // 10 on TM-00-04
#define AUX_C_OUTPUT                        channel_c.sctAuxChannel

void beginAnimationAtCurrentFrame();
void continueAnimation();
bool updateChannelAnimation(neopixel_sct_status_t channel);
void animation_complete();

tm_event animation_complete_event = TM_EVENT_INIT(animation_complete); 

void LEDDRIVER_open (void)
{
  uint32_t clocksPerBit;
  uint32_t prescaler;

  /* Halt H timer, and configure counting mode and prescaler.
   * Set the prescaler for 25 timer ticks per bit (TODO: take care of rounding etc)
   */
  prescaler = SystemCoreClock / (25 * DATA_SPEED);    //(Assume SCT clock = SystemCoreClock)
  LPC_SCT->CTRL_H = 0
      | (0 << SCT_CTRL_H_DOWN_H_Pos)
      | (0 << SCT_CTRL_H_STOP_H_Pos)
      | (1 << SCT_CTRL_H_HALT_H_Pos)      /* HALT counter */
      | (1 << SCT_CTRL_H_CLRCTR_H_Pos)
      | (0 << SCT_CTRL_H_BIDIR_H_Pos)
      | (((prescaler - 1) << SCT_CTRL_H_PRE_H_Pos) & SCT_CTRL_H_PRE_H_Msk);
      ;

  /* Start state */
  LPC_SCT->STATE_H = BITS_PER_INTERRUPT;

  /* Counter LIMIT */
  LPC_SCT->LIMIT_H = (1u << PERIOD_EVENT_NUM);          /* Event 15 */

  /* Configure the match registers */
  clocksPerBit = SystemCoreClock / (prescaler * DATA_SPEED);
  LPC_SCT->MATCHREL_H[0] = clocksPerBit - 1;              /* Bit period */
  LPC_SCT->MATCHREL_H[1] = 8 - 1;          /* T0H */
  LPC_SCT->MATCHREL_H[2] = 16 - 1;  /* T1H */

  /* Configure events */
  LPC_SCT->EVENT[PERIOD_EVENT_NUM].CTRL = 0
      | (0 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH0_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      | (0 << SCT_EVx_CTRL_STATELD_Pos)   /* Add value to STATE */
      | (31 << SCT_EVx_CTRL_STATEV_Pos)   /* Add 31 (i.e subtract 1) */
      ;
  LPC_SCT->EVENT[T1H_EVENT_NUM].CTRL = 0
      | (2 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH2_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      ;
  LPC_SCT->EVENT[T0H_EVENT_NUM].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      ;

  LPC_SCT->RES = 0;

  for (int i = 0; i < MAX_SCT_CHANNELS; i++) {
    if (sct_animation_channels[i]->animationStatus != NULL) {

      LPC_SCT->OUTPUT &= ~(0 
        | (1u << sct_animation_channels[i]->sctOutputChannel)
      );
      LPC_SCT->OUTPUT |= (1u << sct_animation_channels[i]->sctAuxChannel);
      
      LPC_SCT->EVENT[sct_animation_channels[i]->sctOuputBuffer].CTRL = 0
        | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
        | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
        | (1 << SCT_EVx_CTRL_OUTSEL_Pos)    /* Use OUTPUT for I/O condition */
        | (sct_animation_channels[i]->sctAuxChannel << SCT_EVx_CTRL_IOSEL_Pos)    /* Use AUX signal */
        | (0 << SCT_EVx_CTRL_IOCOND_Pos)    /* AUX = 0 */
        | (3 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH AND I/O */
        ;
      LPC_SCT->EVENT[sct_animation_channels[i]->sctAuxBuffer].CTRL = 0
        | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
        | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
        | (1 << SCT_EVx_CTRL_OUTSEL_Pos)    /* Use OUTPUT for I/O condition */
        | (sct_animation_channels[i]->sctAuxChannel << SCT_EVx_CTRL_IOSEL_Pos)    /* Use AUX signal */
        | (3 << SCT_EVx_CTRL_IOCOND_Pos)    /* AUX = 1 */
        | (3 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH AND I/O */
        ;

      LPC_SCT->EVENT[sct_animation_channels[i]->sctOuputBuffer].STATE = 0;
      LPC_SCT->EVENT[sct_animation_channels[i]->sctAuxBuffer].STATE = 0; 

      LPC_SCT->OUT[sct_animation_channels[i]->sctAuxChannel].SET = 0
          | (1u << COMPLETE_FRAME_EVENT)                        /* Complete Event toggles the AUX signal */
          ;
      LPC_SCT->OUT[sct_animation_channels[i]->sctAuxChannel].CLR = 0
          | (1u << COMPLETE_FRAME_EVENT)                        /* Complete Event toggles the AUX signal */
          ;
      LPC_SCT->OUT[sct_animation_channels[i]->sctOutputChannel].SET = 0
          | (1u << PERIOD_EVENT_NUM)                        /* Event 15 sets the DATA signal */
          | (1u << sct_animation_channels[i]->sctOuputBuffer)                        /* Event 12 sets the DATA signal */
          | (1u << sct_animation_channels[i]->sctAuxBuffer)                        /* Event 11 sets the DATA signal */
          ;
      LPC_SCT->OUT[sct_animation_channels[i]->sctOutputChannel].CLR = 0
          | (1u << T1H_EVENT_NUM)                        /* Event 14 clears the DATA signal */
          | (1u << T0H_EVENT_NUM)                        /* Event 13 clears the DATA signal */
          | (1u << COMPLETE_FRAME_EVENT)                        /* Complete Event clears the DATA signal */
          ;

      LPC_SCT->RES |= 0
            | (0 << 2 * sct_animation_channels[i]->sctOutputChannel)            /* DATA signal doesn't change */
            | (3 << 2 * sct_animation_channels[i]->sctAuxChannel) 
            ;
    }
  }

  LPC_SCT->EVENT[COMPLETE_FRAME_EVENT].CTRL = 0
      | (2 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH2_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      | (1 << SCT_EVx_CTRL_STATELD_Pos)   /* Set STATE to a value */
      | (BITS_PER_INTERRUPT << SCT_EVx_CTRL_STATEV_Pos)   /* Set to 8 */
      ;
  LPC_SCT->EVENT[PERIOD_EVENT_NUM].STATE = 0xFFFFFFFF;
  LPC_SCT->EVENT[T1H_EVENT_NUM].STATE = 0x00FFFFFE;  /* All data bit states except state 0 */
  LPC_SCT->EVENT[T0H_EVENT_NUM].STATE = 0x00FFFFFF;  /* All data bit states */
  LPC_SCT->EVENT[COMPLETE_FRAME_EVENT].STATE = 0x00000001; /* Only in state 0 */

      /* Default is to halt the block transfer after the next frame */
  LPC_SCT->HALT_H = (1u << COMPLETE_FRAME_EVENT);           /* Complete Event halts the transfer */

  // Clear pending interrupts on period completion
  LPC_SCT->EVFLAG = (1u << COMPLETE_FRAME_EVENT);
  /* Configure interrupt events */
  LPC_SCT->EVEN |= (1u << COMPLETE_FRAME_EVENT);            /* Complete Event */
}



/* Simple function to write to a transmit buffer. */
void LEDDRIVER_writeNextRGBValue (neopixel_sct_status_t sct_channel)
{
  
  // Make sure this channel actually has animations
  if (sct_channel.animationStatus != NULL) {
    // Get the current rgb value
    uint32_t rgb = (sct_channel.animationStatus->animation.frames[sct_channel.animationStatus->framesSent][sct_channel.animationStatus->bytesSent] << 16
                 | sct_channel.animationStatus->animation.frames[sct_channel.animationStatus->framesSent][sct_channel.animationStatus->bytesSent + 1] << 8
                 | sct_channel.animationStatus->animation.frames[sct_channel.animationStatus->framesSent][sct_channel.animationStatus->bytesSent + 2]);
    // Set the rgb value to the appropriate state
    if (LPC_SCT->OUTPUT & (1u << sct_channel.sctAuxChannel)) {
      LPC_SCT->EVENT[sct_channel.sctOuputBuffer].STATE = (rgb & 0xFFFFFF);
    }
    else {
      LPC_SCT->EVENT[sct_channel.sctAuxBuffer].STATE = (rgb & 0xFFFFFF);
    }
  }
}

/* Activate or deactivate HALT after next frame. */
void LEDDRIVER_haltAfterFrame (int on)
{
  if (on) {
    LPC_SCT->HALT_H = (1u << COMPLETE_FRAME_EVENT);
  }
  else {
    LPC_SCT->HALT_H = 0;
  }
}

/* Start a block transmission */
void LEDDRIVER_start (void)
{
  /* TODO: Check whether timer is really in HALT mode */

  /* Set reset time */
  LPC_SCT->COUNT_H = - LPC_SCT->MATCHREL_H[0] * 50;     /* TODO: Modify this to guarantee 50 µs min in both modes! */

  /* Start state */
  LPC_SCT->STATE_H = BITS_PER_INTERRUPT;

  /* Start timer H */
  LPC_SCT->CTRL_H &= ~SCT_CTRL_H_HALT_H_Msk;
}

void SCT_IRQHandler (void)
{
  /* Acknowledge interrupt */
  if (LPC_SCT->EVFLAG & (1u << COMPLETE_FRAME_EVENT)) {
    // Unset IRQ flag
    LPC_SCT->EVFLAG = (1u << COMPLETE_FRAME_EVENT);

    // Bool to track if all animations are complete
    bool complete = true;
    // Iterate through SCT channels
    for (int i = 0; i < MAX_SCT_CHANNELS; i++) {

      // Attempt to update their channel
      if (updateChannelAnimation(*(sct_animation_channels[i]))) {
        // If we did, then we aren't complete with sending data
        complete = false;
      }
    }

    // If we finished sending data
    if (complete) {
      // Trigger the callback
      tm_event_trigger(&animation_complete_event);
    }
  }
}

// Updates the SCT with the next byte of an animation
// Returns the number of bytes updated (0 or 1)
bool updateChannelAnimation(neopixel_sct_status_t channel) {

  bool byteSent = false;

  // If this channel doesn't have an animation, then we can return
  if (channel.animationStatus == NULL) return byteSent;

  channel.animationStatus->bytesSent+=3;

  // If we have not yet sent all of our frames
  if (channel.animationStatus->framesSent < channel.animationStatus->animation.numFrames) {

    // If we have not yet sent all of our bytes in the current frame
    if (channel.animationStatus->bytesSent <= channel.animationStatus->animation.frameLength) {

      // Send the next byte
      LEDDRIVER_writeNextRGBValue(channel); 

      byteSent = true;

      uint32_t bytesRemaining = channel.animationStatus->animation.frameLength - channel.animationStatus->bytesSent;

      // If we only have one byte left (but it's double buffered, so the 2nd last byte is current being sent)
      if (bytesRemaining == 3) {

        // We're going to halt 
        LEDDRIVER_haltAfterFrame(1);

      }

      // If we have sent all of the bytes in this frame (bytesSent is pre-)
      else if (bytesRemaining == 0) {

        // Move onto the next
        channel.animationStatus->framesSent++;
        channel.animationStatus->bytesSent = 0;

        // If we have now sent all of them
        if (channel.animationStatus->framesSent == channel.animationStatus->animation.numFrames) {
          
          // Trigger the end
          byteSent = false;
        }

        // If not all frames have been sent
        else {
          // Continue with the next frame
          continueAnimation();

          byteSent = true;
        }
      }
    }
  }

  return byteSent;
}

void neopixel_reset_animation() {

  // Disable the SCT IRQ
  NVIC_DisableIRQ(SCT_IRQn);

  // We should make sure the SCT is halted
  LEDDRIVER_haltAfterFrame(true);

  // Make sure the Lua state exists
  lua_State* L = tm_lua_state;
  if (!L) return;

  for (int i = 0; i < MAX_SCT_CHANNELS; i++) {
      // If we have an active animation
    if (sct_animation_channels[i]->animationStatus != NULL &&
      sct_animation_channels[i]->animationStatus->animation.numFrames != 0) {
      // Unreference our buffer so it can be garbage collected
      luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, sct_animation_channels[i]->animationStatus->animation.frameRef);

      // Free our animation buffers and struct memory
      free(sct_animation_channels[i]->animationStatus->animation.frames);
      
      free((neopixel_animation_status_t *)sct_animation_channels[i]->animationStatus);
      sct_animation_channels[i]->animationStatus = NULL;
    }

  }
  // Unreference the event
  tm_event_unref(&animation_complete_event);
}

void animation_complete() {
  // Reset all of our variables
  neopixel_reset_animation();

  lua_State* L = tm_lua_state;
  if (!L) return;
  // Push the _colony_emit helper function onto the stack
  lua_getglobal(L, "_colony_emit");
  // The process message identifier
  lua_pushstring(L, "neopixel_animation_complete");
  // Call _colony_emit to run the JS callback
  tm_checked_call(L, 1);
}

void continueAnimation() {

  LEDDRIVER_writeNextRGBValue(channel_a);

  // Don't halt
  LPC_SCT->HALT_H = 0;

  // Set the flag to get an interrupt when byte transfer is complete
  LPC_SCT->EVEN |= (1u << COMPLETE_FRAME_EVENT);

   /* Set reset time */
  LPC_SCT->COUNT_H = - LPC_SCT->MATCHREL_H[0] * 50;     /* TODO: Modify this to guarantee 50 µs min in both modes! */

  // Set the state to the default
  LPC_SCT->STATE_H = BITS_PER_INTERRUPT;

  /* Start timer H */
  LPC_SCT->CTRL_H &= ~SCT_CTRL_H_HALT_H_Msk;
}

void beginAnimation() {
  // Initialize the LEDDriver
  LEDDRIVER_open();

  // Allow SCT IRQs (which update the relevant data byte)
  NVIC_EnableIRQ(SCT_IRQn);

  // Fill the double buffer with the first two bytes
  for (int i = 0; i < MAX_SCT_CHANNELS; i++) {
    LEDDRIVER_writeNextRGBValue(*(sct_animation_channels[i]));
    LPC_SCT->OUTPUT &= ~(1u << sct_animation_channels[i]->sctAuxChannel);
    LEDDRIVER_writeNextRGBValue(*(sct_animation_channels[i]));
  }
  
  // Do not halt after the first frame
  LEDDRIVER_haltAfterFrame(0); 

  // Start the operation
  LEDDRIVER_start();
}

void setPinSCTFunc(uint8_t pin) {
  // Set up the pin as SCT out
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    g_APinDescription[pin].mode,
    g_APinDescription[pin].alternate_func);
}

int8_t writeAnimationBuffers(neopixel_animation_status_t **channel_animations) {

  // Bool indicating whether any channels have animations ready
  bool animationsReady = false;

  // For each SCT channel
  for (int i = 0; i < MAX_SCT_CHANNELS; i++) {

    // If this channel has animation frames
    if (channel_animations[i] != NULL) {
      // Assign the data buffers to the memory struct
      sct_animation_channels[i]->animationStatus = channel_animations[i];

      // Set up the pin as SCT
      setPinSCTFunc(sct_animation_channels[i]->pin);

      animationsReady = true;
    }
  }

  if (!animationsReady) {
    return -1;
  }

  // Set the system core clock to 180MHz
  SystemCoreClock = 180000000;

  /* Then start transmission */
  beginAnimation();

  // Hold the event queue open until we're done with this event
  tm_event_ref(&animation_complete_event);

  return 0;
}
