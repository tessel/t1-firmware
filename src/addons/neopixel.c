#include "neopixel.h" 

/* TODO: Definitions for example code */
#define DATA_OUTPUT                         8
#define AUX_OUTPUT                          1
#define DUMMY_OUTPUT                        2
#define DATA_SPEED                          800000

volatile int frameCount;
volatile int patternIndex = 0;
volatile uint32_t *outputData;

uint32_t massageBuffer(const uint8_t *uncompressedBuffer, uint32_t uncompressedLength);

void LEDDRIVER_open (void)
{
    uint32_t clocksPerBit;
    uint32_t prescaler;

    /* Halt H timer, and configure counting mode and prescaler.
     * Set the prescaler for 25 timer ticks per bit (TODO: take care of rounding etc)
     */
//    prescaler = CLKPWR_getBusClock(CLKPWR_CLOCK_SCT) / (25 * DATA_SPEED);
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
    LPC_SCT->STATE_H = 24;

    /* Counter LIMIT */
    LPC_SCT->LIMIT_H = (1u << 15);          /* Event 15 */

    /* Configure the match registers */
//    clocksPerBit = CLKPWR_getBusClock(CLKPWR_CLOCK_SCT) / (prescaler * DATA_SPEED);
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
        | (24 << SCT_EVx_CTRL_STATEV_Pos)   /* Set to 24 */
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

    /* Configure interrupt events */
    LPC_SCT->EVFLAG = 1u << 10;
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
  int continueTX = 0;

  /* Acknowledge interrupt */
  LPC_SCT->EVFLAG = (1u << 10);

  if(LPC_SCT->STATE_H == 24) {
      LPC_GPIO_PORT->NOT[4] = (1u << 13);
  }

  if (frameCount) {
      --frameCount;
      ++patternIndex;
      if (frameCount > 0) {
          LEDDRIVER_writeRGB(outputData[patternIndex]);

          continueTX = 1;
      }
  }

  if (!continueTX) {
      LEDDRIVER_haltAfterFrame(1);
  }
}

int8_t writeAnimationBuffer(const uint8_t *buffer, uint32_t buffer_size) {

  if (buffer_size <= 0) {
    return -1;
  }

  uint32_t massagedLen = massageBuffer(buffer, buffer_size);

  uint8_t pin = E_G4;
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    g_APinDescription[pin].mode,
    g_APinDescription[pin].alternate_func);
    SystemCoreClock = 180000000;

  // Initialize the LEDDriver
  LEDDRIVER_open();
  // Allow SCT IRQs (which update the relevant data byte)
  NVIC_EnableIRQ(SCT_IRQn);

  /* Send block of frames */
  /* Preset first data word */
  LEDDRIVER_writeRGB(outputData[0]);
  frameCount = massagedLen;
  /* Then start transmission */
  LEDDRIVER_haltAfterFrame(0);
  LEDDRIVER_start();

  // TIM_Waitms(3);
  // for (uint32_t i = 0; i < massagedLen; i++) {
  //   TM_DEBUG("And now we're going to send %d at %d", outputData[i], i);
  // }

  return 0;
}

uint32_t massageBuffer(const uint8_t *uncompressedBuffer, uint32_t uncompressedLength) {
  // We're going to take a uint8_t buffer and compress
  // the grb values together
  uint32_t packedBufferLen = uncompressedLength/3;
  outputData = malloc(packedBufferLen);

  uint32_t pixel = 0;
  for (uint32_t i = 0; i < uncompressedLength; i+=3) {
    outputData[pixel++] = uncompressedBuffer[i] << 16 |
                    uncompressedBuffer[i + 1] << 8 |
                    uncompressedBuffer[i + 2];
  }

  return packedBufferLen;
}
