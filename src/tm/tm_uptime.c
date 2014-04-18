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

}

uint32_t tm_uptime_micro ()
{
    return LPC_TIMER1->TC;
}

void hw_timer_update_interrupt()
{

}

