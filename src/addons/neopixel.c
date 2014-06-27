#include "neopixel.h" 

/* TODO: Definitions for example code */
#define DATA_OUTPUT                         8
#define AUX_OUTPUT                          1
#define DUMMY_OUTPUT                        2
#define DATA_SPEED                          800000
#define BITS_PER_INTERRUPT                  8 // Could also be 24 if we wanted

volatile struct neopixel_status_t neopixelStatus = {
  .animation = {
    .frames = NULL,
    .frameLengths = NULL,
    .frameRefs = NULL,
    .numFrames = 0,
  },
  .bytesSent = 0,
  .framesSent = 0,
};

void beginAnimationAtCurrentFrame();
// void massageBuffer(const uint8_t *uncompressedBuffer, uint32_t uncompressedLength);
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

  /* Preset the SCTOUTx signals */
  LPC_SCT->OUTPUT &= ~(0
      | (1u << DATA_OUTPUT)
      | (1u << AUX_OUTPUT)
      | (1u << DUMMY_OUTPUT)
      );

  /* Start state */
  LPC_SCT->STATE_H = BITS_PER_INTERRUPT;

  /* Counter LIMIT */
  LPC_SCT->LIMIT_H = (1u << 15);          /* Event 15 */

  /* Configure the match registers */
  clocksPerBit = SystemCoreClock / (prescaler * DATA_SPEED);
  LPC_SCT->MATCHREL_H[0] = clocksPerBit - 1;              /* Bit period */
  LPC_SCT->MATCHREL_H[1] = 8 - 1;          /* T0H */
  LPC_SCT->MATCHREL_H[2] = 16 - 1;  /* T1H */

  /* Configure events */
  LPC_SCT->EVENT[15].CTRL = 0
      | (0 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH0_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      | (0 << SCT_EVx_CTRL_STATELD_Pos)   /* Add value to STATE */
      | (31 << SCT_EVx_CTRL_STATEV_Pos)   /* Add 31 (i.e subtract 1) */
      ;
  LPC_SCT->EVENT[14].CTRL = 0
      | (2 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH2_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      ;
  LPC_SCT->EVENT[13].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      ;
  LPC_SCT->EVENT[12].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_OUTSEL_Pos)    /* Use OUTPUT for I/O condition */
      | (AUX_OUTPUT << SCT_EVx_CTRL_IOSEL_Pos)    /* Use AUX signal */
      | (0 << SCT_EVx_CTRL_IOCOND_Pos)    /* AUX = 0 */
      | (3 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH AND I/O */
      ;
  LPC_SCT->EVENT[11].CTRL = 0
      | (1 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH1_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_OUTSEL_Pos)    /* Use OUTPUT for I/O condition */
      | (AUX_OUTPUT << SCT_EVx_CTRL_IOSEL_Pos)    /* Use AUX signal */
      | (3 << SCT_EVx_CTRL_IOCOND_Pos)    /* AUX = 1 */
      | (3 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH AND I/O */
      ;
  LPC_SCT->EVENT[10].CTRL = 0
      | (2 << SCT_EVx_CTRL_MATCHSEL_Pos)  /* MATCH2_H */
      | (1 << SCT_EVx_CTRL_HEVENT_Pos)    /* Belongs to H counter */
      | (1 << SCT_EVx_CTRL_COMBMODE_Pos)  /* MATCH only */
      | (1 << SCT_EVx_CTRL_STATELD_Pos)   /* Set STATE to a value */
      | (BITS_PER_INTERRUPT << SCT_EVx_CTRL_STATEV_Pos)   /* Set to 8 */
      ;
  LPC_SCT->EVENT[15].STATE = 0xFFFFFFFF;
  LPC_SCT->EVENT[14].STATE = 0x00FFFFFE;  /* All data bit states except state 0 */
  LPC_SCT->EVENT[13].STATE = 0x00FFFFFF;  /* All data bit states */
  LPC_SCT->EVENT[12].STATE = 0;
  LPC_SCT->EVENT[11].STATE = 0;
  LPC_SCT->EVENT[10].STATE = 0x00000001;  /* Only in state 0 */

      /* Default is to halt the block transfer after the next frame */
  LPC_SCT->HALT_H = (1u << 10);           /* Event 10 halts the transfer */

  /* Output actions (TODO: honor previous register settings) */
  LPC_SCT->OUT[AUX_OUTPUT].SET = 0
      | (1u << 10)                        /* Event 10 toggles the AUX signal */
      ;
  LPC_SCT->OUT[AUX_OUTPUT].CLR = 0
      | (1u << 10)                        /* Event 10 toggles the AUX signal */
      ;
  LPC_SCT->OUT[DATA_OUTPUT].SET = 0
      | (1u << 15)                        /* Event 15 sets the DATA signal */
      | (1u << 12)                        /* Event 12 sets the DATA signal */
      | (1u << 11)                        /* Event 11 sets the DATA signal */
      ;
  LPC_SCT->OUT[DATA_OUTPUT].CLR = 0
      | (1u << 14)                        /* Event 14 clears the DATA signal */
      | (1u << 13)                        /* Event 13 clears the DATA signal */
      | (1u << 10)                        /* Event 10 clears the DATA signal */
      ;

  /* Conflict resolution (TODO: honor previous register settings) */
  LPC_SCT->RES = 0
      | (0 << 2 * DATA_OUTPUT)            /* DATA signal doesn't change */
      | (3 << 2 * AUX_OUTPUT)             /* AUX signal toggles */
      ;

  // Clear pending interrupts on period completion
  LPC_SCT->EVFLAG = (1u << 10);
  /* Configure interrupt events */
  LPC_SCT->EVEN |= (1u << 10);            /* Event 10 */
}



/* Simple function to write to a transmit buffer. */
void LEDDRIVER_writeRGB (uint32_t rgb)
{
  if (LPC_SCT->OUTPUT & (1u << AUX_OUTPUT)) {
    LPC_SCT->EVENT[12].STATE = rgb;
  }
  else {
    LPC_SCT->EVENT[11].STATE = rgb;
  }
}



/* Activate or deactivate HALT after next frame. */
void LEDDRIVER_haltAfterFrame (int on)
{
  if (on) {
    LPC_SCT->HALT_H = (1u << 10);
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
  LPC_SCT->STATE_H = 0;

  /* Start timer H */
  LPC_SCT->CTRL_H &= ~SCT_CTRL_H_HALT_H_Msk;
}

void SCT_IRQHandler (void)
{

  /* Acknowledge interrupt */
  LPC_SCT->EVFLAG = (1u << 10);

  // If we have not yet sent all of our frames
  if (neopixelStatus.framesSent < neopixelStatus.animation.numFrames) {

    // If we have not yet sent all of our bytes in the current frame
    if (neopixelStatus.bytesSent < neopixelStatus.animation.frameLengths[neopixelStatus.framesSent]) {

      // Send the next byte
      LEDDRIVER_writeRGB(neopixelStatus.animation.frames[neopixelStatus.framesSent][neopixelStatus.bytesSent++]); 

      // If we only have one byte next
      if (neopixelStatus.animation.frameLengths[neopixelStatus.framesSent] - neopixelStatus.bytesSent == 0) {

        // We're going to halt 
        LEDDRIVER_haltAfterFrame(1);
      }
    }

    // If we have sent all of the bytes in this frame
    if (neopixelStatus.bytesSent == neopixelStatus.animation.frameLengths[neopixelStatus.framesSent]) {
      
      // Move onto the next
      neopixelStatus.framesSent++;
      neopixelStatus.bytesSent = 0;

      // If we have now sent all of them
      if (neopixelStatus.framesSent == neopixelStatus.animation.numFrames) {
        
        // Trigger the end
        tm_event_trigger(&animation_complete_event);
      }

      // If not all frames have been sent
      else {
        // Continue with the next frame
        beginAnimationAtCurrentFrame();
      }
    } 
  }
}

void neopixel_reset_animation() {

  // Disable the SCT IRQ
  NVIC_DisableIRQ(SCT_IRQn);

  // We should make sure the SCT is halted
  LEDDRIVER_haltAfterFrame(true);

  // Make sure the Lua state exists
  lua_State* L = tm_lua_state;
  if (!L) return;

  // If we have an active animation
  if (neopixelStatus.animation.numFrames != 0) {
    // Iterate through all of our references
    for (uint32_t i = 0; i < neopixelStatus.animation.numFrames; i++) {
      // Unreference our buffer so it can be garbage collected
      luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, neopixelStatus.animation.frameRefs[i]);
    }

    // Free our animation buffers
    free(neopixelStatus.animation.frames);
    free(neopixelStatus.animation.frameLengths);
    free(neopixelStatus.animation.frameRefs);
    neopixelStatus.animation.numFrames = 0;
  }
  
  // Reset global counters
  neopixelStatus.bytesSent = 0;
  neopixelStatus.framesSent = 0;

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

void beginAnimationAtCurrentFrame() {
  // Initialize the LEDDriver
  LEDDRIVER_open();

  // Allow SCT IRQs (which update the relevant data byte)
  NVIC_EnableIRQ(SCT_IRQn);

  /* Send block of frames */
  /* Preset first data word */
  LEDDRIVER_writeRGB(neopixelStatus.animation.frames[neopixelStatus.framesSent][neopixelStatus.bytesSent++]);
  
  // Do not halt after the first frame
  LEDDRIVER_haltAfterFrame(0); 

  // Start the operation
  LEDDRIVER_start();
}

int8_t writeAnimationBuffer(const uint8_t **frames, int32_t *frameRefs, uint32_t *frameLengths, uint32_t numFrames) {

  if (numFrames <= 0) {
    return -1;
  }

  uint8_t pin = E_G4;

  // Initialize buffers
  neopixelStatus.animation.frames = frames;
  neopixelStatus.animation.frameRefs = frameRefs;
  neopixelStatus.animation.frameLengths = frameLengths;
  neopixelStatus.animation.numFrames = numFrames;
  neopixelStatus.framesSent = 0;
  neopixelStatus.bytesSent = 0;

  // Set up the pin as SCT out
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    g_APinDescription[pin].mode,
    g_APinDescription[pin].alternate_func);
    SystemCoreClock = 180000000;

  /* Then start transmission */
  beginAnimationAtCurrentFrame();

  // Hold the event queue open until we're done with this event
  tm_event_ref(&animation_complete_event);

  return 0;
}
