// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "hw.h"
#include "variant.h"
#include "LPC18xx.h"
#include "lpc18xx_sct.h"

#define SCT_CONFIG_UNIFY (1 << 0)
#define SCT_CTRL_HALT (1 << 2)
#define SCT_CTRL_CLEAR (1 << 3)
#define SCT_EVENT_CTRL_MATCH(x) (x << 0)
#define SCT_EVENT_CTRL_MATCH_ONLY (1 << 12)

int hw_pwm_enable (size_t pin, hw_pwm_t mode, uint32_t period, uint32_t pulsewidth)
{
  // pin is currently unused
  (void) pin;
  (void) mode;

  // Halt the SCT
  LPC_SCT->CTRL_H = SCT_CTRL_HALT;
  LPC_SCT->CTRL_L = SCT_CTRL_HALT;

  if (g_APinDescription[pin].alternate != PWM_MODE) {
    return -1; // Not a PWM pin
  }

  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    g_APinDescription[pin].mode,
    g_APinDescription[pin].alternate_func);

  LPC_SCT->CONFIG = SCT_CONFIG_UNIFY; // 32bit, Clocked from peripheral clock
  LPC_SCT->REGMODE_L = 0;     // All registers are match registers

  // Event 0 at counter period
  LPC_SCT->EVENT[0].CTRL = SCT_EVENT_CTRL_MATCH(0) | SCT_EVENT_CTRL_MATCH_ONLY; // match register 0, match condition only, no state change
  LPC_SCT->EVENT[0].STATE = (1 << 0); // in state 0
  LPC_SCT->MATCH[0].U = period;
  LPC_SCT->MATCHREL[0].U = period;

  // Event 1 at counter match
  LPC_SCT->EVENT[1].CTRL = SCT_EVENT_CTRL_MATCH(1) | SCT_EVENT_CTRL_MATCH_ONLY; // match register 1, match condition only, no state change
  LPC_SCT->EVENT[1].STATE = (1 << 0); // in state 0
  LPC_SCT->MATCH[1].U = pulsewidth;
  LPC_SCT->MATCHREL[1].U = pulsewidth;

  // Event 0 resets the counter
  LPC_SCT->LIMIT_L = (1 << 0);

  LPC_SCT->OUT[g_APinDescription[pin].pwm_channel].SET = (1 << 0); // Event 0 sets the output
  LPC_SCT->OUT[g_APinDescription[pin].pwm_channel].CLR = (1 << 1); // Event 1 clears the output

  // Start in state 0
  LPC_SCT->STATE_L = 0;

  // Clear the counter and un-halt the SCT;
  LPC_SCT->CTRL_U = SCT_CTRL_CLEAR;

  return 0;
}
