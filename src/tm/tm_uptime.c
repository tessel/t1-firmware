/*
 * tm_time.c
 *
 *  Created on: Aug 12, 2013
 *      Author: tim
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "lpc18xx_timer.h"
#include "tm.h"


void tm_uptime_init()
{
        TIM_TIMERCFG_Type TIM_ConfigStruct;

        // Initialize timer 0, prescale count time of 1000000uS = 1S
        TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
        TIM_ConfigStruct.PrescaleValue        = 1;

        // Set configuration for Tim_config and Tim_MatchConfig
        TIM_Init(LPC_TIMER1, TIM_TIMER_MODE, &TIM_ConfigStruct);
        TIM_ResetCounter(LPC_TIMER1);
        // To start timer 0
        TIM_Cmd(LPC_TIMER1,ENABLE);
}

uint32_t tm_uptime_micro ()
{
        return LPC_TIMER1->TC;
}

void hw_timer_update_interrupt()
{

}

#ifdef __cplusplus
}
#endif
