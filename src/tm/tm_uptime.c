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

void tm_timestamp_wrapped();

#define TIMER LPC_TIMER3
#define TIMER_CHAN_OVF 0
#define TIMER_CHAN_EVT 1
#define TIMER_MCR(x) (1 << (x * 3))
#define TIMER_IR(x) (1 << x)

void tm_uptime_init()
{
    CGU_ConfigPWR(CGU_PERIPHERAL_TIMER3, ENABLE);

    TIMER->PR = 180;
    TIMER->IR = 0xFFFFFFFF; // clear interrupts

    // Match on overflow used to increment base
    TIMER->MR[TIMER_CHAN_OVF] = 0;
    TIMER->MCR |= TIMER_MCR(TIMER_CHAN_OVF);

    TIMER->TCR = (1<<1); // reset counter
    TIMER->TCR = (1<<0); // release reset and start

    NVIC_EnableIRQ(TIMER3_IRQn);
}

uint32_t tm_uptime_micro ()
{

    return TIMER->TC;
}

void hw_timer_update_interrupt()
{
    if (tm_timer_waiting()) {
        unsigned base = tm_timer_base_time();
        unsigned step = tm_timer_head_time();
        TIMER->MR[TIMER_CHAN_EVT] = base + step;
        TIMER->MCR |= TIMER_MCR(TIMER_CHAN_EVT); // Enable timer match interrupt

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

    TIMER->MCR &= ~TIMER_MCR(TIMER_CHAN_EVT); // Disable timer match interrupt
}

void __attribute__ ((interrupt)) TIMER3_IRQHandler() {
    if (TIMER->IR & TIMER_IR(TIMER_CHAN_EVT)) {
        TIMER->IR = TIMER_IR(TIMER_CHAN_EVT);
        tm_event_trigger(&tm_timer_event);
    }

    if (TIMER->IR & TIMER_IR(TIMER_CHAN_OVF)) {
        TIMER->IR = TIMER_IR(TIMER_CHAN_OVF);
        tm_timestamp_wrapped();
    }
}
