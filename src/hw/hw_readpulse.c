// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "tm.h"
#include "hw.h"
#include "colony.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <variant.h>
#include <lpc18xx_sct.h>

/* System definitions */
#define SYSTEM_CORE_CLOCK                   180000000

/* Event definitions */
#define EVENT_TIMEOUT                       1
#define EVENT_START_TIMING                  2
#define EVENT_END_TIMING                    3

/* State definitions */
#define STATE_TIMING                        0
#define STATE_ACTIVE                        1
#define STATE_RETURN                        2

/* Macro to define register bits and mask in CMSIS style */
#define LPCLIB_DEFINE_REG_BIT(name,pos,width) \
  enum { \
    name##_POS = pos, \
    name##_MSK = (int)(((1ul << width) - 1) << pos), \
  }

/* SCT control positions using CMSIS styler macro */
LPCLIB_DEFINE_REG_BIT(DOWN_L,   0,  1);
LPCLIB_DEFINE_REG_BIT(STOP_L,   1,  1);
LPCLIB_DEFINE_REG_BIT(HALT_L,   2,  1);
LPCLIB_DEFINE_REG_BIT(CLRCTR_L, 3,  1);
LPCLIB_DEFINE_REG_BIT(BIDIR_L,  4,  1);
LPCLIB_DEFINE_REG_BIT(PRE_L,    5,  8);

/* SCT config positions using CMSIS styler macro */
LPCLIB_DEFINE_REG_BIT(UNIFY,    0,  1);

/* Event control positions using CMSIS styler macro */
LPCLIB_DEFINE_REG_BIT(MATCHSEL, 0,  4);
LPCLIB_DEFINE_REG_BIT(HEVENT,   4,  1);
LPCLIB_DEFINE_REG_BIT(OUTSEL,   5,  1);
LPCLIB_DEFINE_REG_BIT(IOSEL,    6,  4);
LPCLIB_DEFINE_REG_BIT(IOCOND,   10, 2);
LPCLIB_DEFINE_REG_BIT(COMBMODE, 12, 2);
LPCLIB_DEFINE_REG_BIT(STATELD,  14, 1);
LPCLIB_DEFINE_REG_BIT(STATEV,   15, 5);

/* Prototypes */
void sct_set_scu_pin(uint8_t pin);
void sct_driver_init (const char* type, uint32_t timeout);
void sct_driver_start (void);
void sct_pulse_read_complete ();
void sct_irq_handler (void);

/* Event triggered when read pulse is complete */
tm_event sct_pulse_read_complete_event = TM_EVENT_INIT(sct_pulse_read_complete);


/* Reads a pulse in and returns the duration of the pulse */
uint8_t sct_read_pulse (const char* type, uint32_t timeout)
{
  // set the pin configuration
  sct_set_scu_pin(E_G3);

  // hold the event queue open until we're done
  tm_event_ref(&sct_pulse_read_complete_event);

  // initialize the driver
  sct_driver_init(type, timeout);

  // allow SCT IRQs
  NVIC_EnableIRQ(SCT_IRQn);

  // start the driver
  sct_driver_start();

  // successful return
  return 0;
}


/* Sets up the pins */
void sct_set_scu_pin(uint8_t pin)
{
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    PUP_DISABLE | PDN_DISABLE,
    g_APinDescription[pin].alternate_func);
}


/* Configures the driver for the SCT */
void sct_driver_init (const char* type, uint32_t timeout)
{
  // 1 = rising, 2 = falling (UM10430.pdf - page 849 - IOCOND)
  int type_start = (type[0] == 'h') ? 0x1 : 0x2;
  int type_end   = (type[0] == 'h') ? 0x2 : 0x1;

  // set the sct control register
  LPC_SCT->CTRL_L = 0
		| ( 0 << DOWN_L_POS )
		| ( 0 << STOP_L_POS )
		| ( 1 << HALT_L_POS )
		| ( 1 << CLRCTR_L_POS )
		| ( 0 << BIDIR_L_POS )
    ;

  // set the start state
  LPC_SCT->STATE_L = STATE_TIMING;

  // set the match register and the two capture resgisters
  LPC_SCT->REGMODE_L = ( 0 << EVENT_TIMEOUT ) | ( 1 << EVENT_END_TIMING );

  // set the config to be a unity 32 bit counter (timeout is a uint32)
  LPC_SCT->CONFIG |= (1 << UNIFY_POS);

  // set the timeout as a match limit
  LPC_SCT->MATCH_L[0] = timeout * SYSTEM_CORE_CLOCK;
  LPC_SCT->MATCHREL_L[0] = timeout * SYSTEM_CORE_CLOCK;

  // set the end timing event to capture the counter value
  LPC_SCT->CAPCTRL_L[0] = ( 1 << EVENT_END_TIMING );

  // let the timeout and start timing event limit the counter
  LPC_SCT->LIMIT_L = ( 1 << EVENT_TIMEOUT ) | ( 1 << EVENT_START_TIMING );

  // start timing event state and control
  LPC_SCT->EVENT[EVENT_START_TIMING].STATE = STATE_TIMING;
  LPC_SCT->EVENT[EVENT_START_TIMING].CTRL = 0
    | ( 0 << IOSEL_POS )
    | ( type_start << IOCOND_POS )
    | ( 3 << COMBMODE_POS )
    | ( 0 << STATELD_POS )
    | ( 1 << STATEV_POS )
    ;

  // end timing event state and control
  LPC_SCT->EVENT[EVENT_END_TIMING].STATE = STATE_ACTIVE;
  LPC_SCT->EVENT[EVENT_END_TIMING].CTRL = 0
    | ( 1 << OUTSEL_POS )
    | ( 0 << IOSEL_POS )
    | ( type_end << IOCOND_POS )
    | ( 3 << COMBMODE_POS )
    | ( 0 << STATELD_POS )
    | ( 1 << STATEV_POS )
    ;

  // timeout event state and control
  LPC_SCT->EVENT[EVENT_TIMEOUT].STATE = STATE_TIMING;
  LPC_SCT->EVENT[EVENT_TIMEOUT].CTRL = 0
    | ( 0 << MATCHSEL_POS )
    | ( 1 << OUTSEL_POS )
    | ( 0 << IOSEL_POS )
    | ( 3 << COMBMODE_POS )
    | ( 0 << STATELD_POS )
    | ( 2 << STATEV_POS )
    ;

  // end timing sets out 0
  LPC_SCT->OUT[0].SET = (1 << EVENT_END_TIMING);

  // a timeout clears out 0
  LPC_SCT->OUT[0].CLR = (1 << EVENT_TIMEOUT);

  // sets the output to be out 0
  LPC_SCT->OUTPUT |= 0;

  // halt if the pulse ended or there was a timeout
  LPC_SCT->HALT_L = (1 << EVENT_END_TIMING) | (1 << EVENT_TIMEOUT);

  // configure interrupt events
  LPC_SCT->EVEN = (1 << EVENT_END_TIMING) | (1 << EVENT_TIMEOUT);
}


/* Starts the actual driver by unhalting the SCT */
void sct_driver_start (void)
{
  LPC_SCT->CTRL_L &= ~( 1 << HALT_L_POS );
}


/* Function called when event has been completed */
void sct_pulse_read_complete ()
{
  // get the Lua state or return if there is no state
  lua_State* L = tm_lua_state;
  if (!L) return;

  // push the _colony_emit helper function onto the stack
  lua_getglobal(L, "_colony_emit");

  // the process message identifier
  lua_pushstring(L, "read_pulse_complete");

  // get the actual pulse time
  uint32_t pulse_time = 0;
  if ( LPC_SCT->OUT[0].CLR & EVENT_TIMEOUT ) { pulse_time = -1; }
  else if ( LPC_SCT->OUT[0].SET & EVENT_END_TIMING ) { pulse_time = (LPC_SCT->COUNT_U / SYSTEM_CORE_CLOCK ); }

  // the number of microseconds the pulse was high
  lua_pushnumber(L, pulse_time);

  // call _colony_emit to run the JS callback
  tm_checked_call(L, 2);
}


/* IRQ handler called if there's an interupt */
void sct_irq_handler (void)
{
  // if there is an interupt set the flag and trigger the complete function
  if (LPC_SCT->EVFLAG & (1 << EVENT_END_TIMING))
  {
    LPC_SCT->EVFLAG = (1 << EVENT_END_TIMING);
    tm_event_trigger(&sct_pulse_read_complete_event);
  }
}


#ifdef __cplusplus
}
#endif
