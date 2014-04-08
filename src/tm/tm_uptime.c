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

void tm_uptime_init()
{
        TIM_TIMERCFG_Type TIM_ConfigStruct;
        TIM_CAPTURECFG_Type TIM_CaptureConfigStruct;

        // Initialize timer 0, prescale count time of 1000000uS = 1S
        TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
        TIM_ConfigStruct.PrescaleValue        = 1;

        // use channel 0, CAPn.CAPTURE_CHANNEL
        TIM_CaptureConfigStruct.CaptureChannel = 0;
        // Enable capture on CAPn.CAPTURE_CHANNEL rising edge
        TIM_CaptureConfigStruct.RisingEdge = ENABLE;
        // Enable capture on CAPn.CAPTURE_CHANNEL falling edge
        TIM_CaptureConfigStruct.FallingEdge = ENABLE;
        // Generate capture interrupt
        TIM_CaptureConfigStruct.IntOnCaption = DISABLE;

        // Set configuration for Tim_config and Tim_MatchConfig
        TIM_Init(LPC_TIMER1, TIM_TIMER_MODE, &TIM_ConfigStruct);
        TIM_ConfigCapture(LPC_TIMER1, &TIM_CaptureConfigStruct);
        TIM_ResetCounter(LPC_TIMER1);
        // To start timer 0
        TIM_Cmd(LPC_TIMER1,ENABLE);
}

uint32_t tm_uptime_micro ()
{
        return LPC_TIMER1->TC;
}

#ifdef __cplusplus
}
#endif
