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

// Includes
#include "tm.h"
#include "hw.h"
#include "colony.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <variant.h>
#include <lpc18xx_sct.h>

// Event definitions
#define EVENT_TIMEOUT           1
#define EVENT_START_TIMING      2
#define EVENT_END_TIMING        3

// State definitions
#define STATE_TIMING            0
#define STATE_ACTIVE            1
#define STATE_RETURN            2

// Input definitions
#define SCT_INPUT_PIN           5

// Macro to define register bits and mask in CMSIS style
#define LPCLIB_DEFINE_REG_BIT(name,pos,width) \
  enum { \
    name##_POS = pos, \
    name##_MSK = (int)(((1ul << width) - 1) << pos), \
  }

// SCT control positions using CMSIS style macro
LPCLIB_DEFINE_REG_BIT(STOP_L,   1,  1);
LPCLIB_DEFINE_REG_BIT(HALT_L,   2,  1);
LPCLIB_DEFINE_REG_BIT(CLRCTR_L, 3,  1);
LPCLIB_DEFINE_REG_BIT(PRE_L,    5,  8);

// SCT config positions using CMSIS style macro
LPCLIB_DEFINE_REG_BIT(UNIFY,    0,  1);

// Event control positions using CMSIS style macro
LPCLIB_DEFINE_REG_BIT(MATCHSEL, 0,  4);
LPCLIB_DEFINE_REG_BIT(IOSEL,    6,  4);
LPCLIB_DEFINE_REG_BIT(IOCOND,   10, 2);
LPCLIB_DEFINE_REG_BIT(COMBMODE, 12, 2);
LPCLIB_DEFINE_REG_BIT(STATELD,  14, 1);
LPCLIB_DEFINE_REG_BIT(STATEV,   15, 5);

// Prototypes
void sct_set_scu_pin(uint8_t pin);
void sct_driver_configure (hw_sct_pulse_type_t type, uint32_t timeout);
void sct_driver_start (void);
void sct_read_pulse_complete ();
void sct_log_registers (void);

// The recorded number of ticks over the pulse
uint32_t pulse_ticks = 0;

// Event triggered when read pulse is complete
tm_event sct_read_pulse_complete_event = TM_EVENT_INIT(sct_read_pulse_complete);


// Begins waiting for pulse in
uint8_t sct_read_pulse (hw_sct_pulse_type_t type, uint32_t timeout)
{
  // really clear the SCT: ( 1 << 5 ) is an LPC18xx.h include workaround
  LPC_RGU->RESET_CTRL1 = ( 1 << 5 );

  // set the pin configuration
  sct_set_scu_pin(E_G3);

  // initialize the driver
  sct_driver_configure(type, timeout);

  // allow SCT IRQs
  NVIC_EnableIRQ(SCT_IRQn);

  // start the driver
  sct_driver_start();

  // hold the event queue open until we're done
  tm_event_ref(&sct_read_pulse_complete_event);

  // successful return
  return 0;
}


// Sets up the input pin
void sct_set_scu_pin(uint8_t pin)
{
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    PUP_DISABLE | PDN_DISABLE | FILTER_ENABLE | INBUF_ENABLE,
    g_APinDescription[pin].alternate_func);
}


// Configures the driver for the SCT
void sct_driver_configure (hw_sct_pulse_type_t type, uint32_t timeout)
{
  // what edge to look for at start and end (rising edge = 1, falling edge = 2)
  int type_start = (type == SCT_PULSE_HIGH) ? 1 : 2;
  int type_end   = (type == SCT_PULSE_HIGH) ? 2 : 1;

  // set the config to be a unity 32 bit counter (timeout is a uint32)
  LPC_SCT->CONFIG |= (1 << UNIFY_POS);

  // set the sct control register
  LPC_SCT->CTRL_U = 0
    | ( 0 << STOP_L_POS )              // clear the stop register
    | ( 1 << HALT_L_POS )              // halt while configuring the SCT
    | ( 1 << CLRCTR_L_POS )            // clear the other bits
    | ( 0 << PRE_L_POS )               // set prescaler to match sys clock
    ;

  // set the match and capture register ( match = 0, capture = 1 )
  LPC_SCT->REGMODE_L = ( 0 << 0 ) | ( 1 << 1 );

  // set the timeout as a match limit
  LPC_SCT->MATCH[0].U = ( timeout * SYSTEM_CORE_CLOCK_MS );

  // start timing event state can start once we start the clock
  LPC_SCT->EVENT[EVENT_START_TIMING].STATE = ( 1 << STATE_TIMING );

  // start timing event control
  LPC_SCT->EVENT[EVENT_START_TIMING].CTRL = 0
    | ( type_start << IOCOND_POS )     // looks for rising or falling edge
    | ( SCT_INPUT_PIN << IOSEL_POS )   // selects the input pin
    | ( 2 << COMBMODE_POS )            // look for IO and not a match
    | ( 1 << STATELD_POS )             // load the state in STATEV
    | ( STATE_ACTIVE << STATEV_POS )   // state to load when event triggers
    ;

  // end timing event state can only happen once we're actively timing
  LPC_SCT->EVENT[EVENT_END_TIMING].STATE = ( 1 << STATE_ACTIVE );

  // end timing event control
  LPC_SCT->EVENT[EVENT_END_TIMING].CTRL = 0
    | ( type_end << IOCOND_POS )       // looks for rising or falling edge
    | ( SCT_INPUT_PIN << IOSEL_POS )   // selects the input pin
    | ( 2 << COMBMODE_POS )            // look for IO and not a match
    | ( 1 << STATELD_POS )             // load the state in STATEV
    | ( STATE_RETURN << STATEV_POS )   // state to load when event triggers
    ;

  // timeouts occur after looking for a rising/falling edge for too long
  LPC_SCT->EVENT[EVENT_TIMEOUT].STATE = ( 1 << STATE_TIMING );

  // timout event control
  LPC_SCT->EVENT[EVENT_TIMEOUT].CTRL = 0
    | ( 0 << MATCHSEL_POS )            // match to value in 0 match register
    | ( 1 << COMBMODE_POS )            // match event, not an IO event
    | ( 1 << STATELD_POS )             // load the state in STATEV
    | ( STATE_RETURN << STATEV_POS )   // state to load when event triggers
    ;

  // set the end timing event to capture the counter value
  LPC_SCT->CAPCTRL[1].U = ( 1 << EVENT_END_TIMING );

  // let the start event reset the counter so end event captures pulse length
  LPC_SCT->LIMIT_L = ( 1 << EVENT_START_TIMING );

  // let the timeout and end timing event halt
  LPC_SCT->HALT_L = ( 1 << EVENT_TIMEOUT ) | ( 1 << EVENT_END_TIMING );

  // let timeout and end time set the interupt flag
  LPC_SCT->EVFLAG = (1 << EVENT_END_TIMING) | (1 << EVENT_TIMEOUT);

  // let timeout and end time trigger interupts
  LPC_SCT->EVEN = (1 << EVENT_END_TIMING) | (1 << EVENT_TIMEOUT);
}


// Starts the actual driver by unhalting the SCT
void sct_driver_start (void)
{
  // set the initial state
  LPC_SCT->STATE_L = STATE_TIMING;

  // start the SCT
  LPC_SCT->CTRL_L &= ~( 1 << HALT_L_POS );
}


// Function called when event has been completed and should exit event queue
void sct_read_pulse_complete ()
{
  // reset the SCT
  sct_read_pulse_reset();

  // get the Lua state or return if there is no state
  lua_State* L = tm_lua_state;
  if (!L) return;

  // push the _colony_emit helper function onto the stack
  lua_getglobal(L, "_colony_emit");

  // the process message identifier
  lua_pushstring(L, "read_pulse_complete");

  // the number of milliseconds the pulse was high
  lua_pushnumber(L, (((double)pulse_ticks)/SYSTEM_CORE_CLOCK_MS_F));

  // reconfigure the pin back to gpio in
  hw_digital_startup(E_G3);

  // call _colony_emit to run the JS callback
  tm_checked_call(L, 2);
}

void sct_read_pulse_reset (void)
{
  // set the SCT state back to inactive
  hw_sct_status = SCT_INACTIVE;

  // disable the SCT IRQ
  NVIC_DisableIRQ(SCT_IRQn);

  // halt the SCT
  LPC_SCT->CTRL_U = ( 1 << HALT_L_POS );

  // really clear the SCT: ( 1 << 5 ) is an LPC18xx.h include workaround
  LPC_RGU->RESET_CTRL1 = ( 1 << 5 );

  // unreference the event
  tm_event_unref(&sct_read_pulse_complete_event);
}


// IRQ handler called if there's an interupt
void sct_readpulse_irq_handler (void)
{
  // interupt triggered by the end of a pulse (set the pulse length)
  if (LPC_SCT->EVFLAG & (1 << EVENT_END_TIMING))
  {
    pulse_ticks = LPC_SCT->CAP[1].U;
    LPC_SCT->EVFLAG = (1 << EVENT_END_TIMING);
  }

  // interupt triggered by a timeout (set the pulse length to zero)
  else if (LPC_SCT->EVFLAG & (1 << EVENT_TIMEOUT))
  {
    pulse_ticks = 0;
    LPC_SCT->EVFLAG = (1 << EVENT_TIMEOUT);
  }

  // trigger the event complete for the event queue
  tm_event_trigger(&sct_read_pulse_complete_event);
}


#ifdef __cplusplus
}
#endif
