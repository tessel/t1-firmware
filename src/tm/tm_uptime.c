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
        TIMER->MR[0] = tm_timer_next_time();
        TIMER->MCR |= 1;
    } else {
        TIMER->MCR &= ~1;
    }
}

void __attribute__ ((interrupt)) TIMER1_IRQHandler() {
    if (TIMER->IR & 1) {
        TIMER->IR = 1;
        tm_event_trigger(&tm_timer_event);
    }
}
