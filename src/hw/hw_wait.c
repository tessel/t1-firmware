/*
 * tm_delay.c
 *
 *  Created on: Jul 8, 2013
 *      Author: tim
 */

#include "hw.h"
#include "lpc18xx_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

void hw_wait_ms (int ms)
{
	TIM_Waitms(ms);
}

void hw_wait_us (int us)
{
	TIM_Waitus(us);
}

#ifdef __cplusplus
}
#endif
