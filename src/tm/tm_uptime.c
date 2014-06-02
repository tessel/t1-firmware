// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "LPC18xx.h"
#include "lpc18xx_cgu.h"
#include "tm.h"

#define TIMER LPC_TIMER1

void tm_uptime_init()
{
    CGU_ConfigPWR(CGU_PERIPHERAL_TIMER1, ENABLE);

    TIMER->PR = 180;
    TIMER->IR = 0xFFFFFFFF; // clear interrupts
    TIMER->TCR = (1<<1); // reset counter
    TIMER->TCR = (1<<0); // release reset and start

    NVIC_EnableIRQ(TIMER1_IRQn);
}

uint32_t tm_uptime_micro ()
{
    return LPC_TIMER1->TC;
}

void hw_timer_update_interrupt()
{
    if (tm_timer_waiting()) {
        unsigned base = tm_timer_base_time();
        unsigned step = tm_timer_head_time();
        TIMER->MR[0] = base + step;
        TIMER->MCR |= 1; // Enable timer match interrupt

        // Compare difference rather than absolute to avoid wrap issues
        unsigned elapsed = tm_uptime_micro() - base;
        if (elapsed < step) {
            return; // We're good. Leave the timer set.
        } else {
            // The time already passed, so trigger it now to avoid losing the
            // wakeup. It's fine if the interrupt triggerred between enabling
            // it and testing it because tm_event_trigger is atomic and
            // idempotent.
            tm_event_trigger(&tm_timer_event);
        }
    }

    TIMER->MCR &= ~1; // Disable timer match interrupt
}

void __attribute__ ((interrupt)) TIMER1_IRQHandler() {
    if (TIMER->IR & 1) {
        TIMER->IR = 1;
        tm_event_trigger(&tm_timer_event);
    }
}
