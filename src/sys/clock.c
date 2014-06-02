// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#include "linker.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_emc.h"

_ramfunc uint32_t	CGU_Init(void)
{
	__disable_irq();
	MemoryPinInit(); // Make sure EMC is in high-speed pin mode

 	/* Set the XTAL oscillator frequency to 12MHz*/
	CGU_SetXTALOSC(12000000);
	CGU_EnableEntity(CGU_CLKSRC_XTAL_OSC, ENABLE);
	CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_BASE_SPIFI);
	CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_BASE_M3);

	/* Set PL160M 12*1 = 12 MHz */
	CGU_EntityConnect(CGU_CLKSRC_XTAL_OSC, CGU_CLKSRC_PLL1);
	CGU_SetPLL1(1);
	CGU_EnableEntity(CGU_CLKSRC_PLL1, ENABLE);

	/* Run SPIFI from PL160M, /2 */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_CLKSRC_IDIVA);
	CGU_EnableEntity(CGU_CLKSRC_IDIVA, ENABLE);
	CGU_SetDIV(CGU_CLKSRC_IDIVA, 2); // This gets adjusted in spi_flash.c to slow the clock when writing
	CGU_EntityConnect(CGU_CLKSRC_IDIVA, CGU_BASE_SPIFI);
	CGU_UpdateClock();

	LPC_CCU1->CLK_M3_EMCDIV_CFG |= (1<<0) |  (1<<5);		// Turn on clock / 2
	LPC_CREG->CREG6 |= (1<<16);	// EMC divided by 2
    LPC_CCU1->CLK_M3_EMC_CFG |= (1<<0);		// Turn on clock

	/* Set PL160M @ 12*9=108 MHz */
	CGU_SetPLL1(9);

	/* Run base M3 clock from PL160M, no division */
	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_BASE_M3);

	emc_WaitMS(30);

	/* Change the clock to 180 MHz */
	/* Set PL160M @ 12*15=180 MHz */
	CGU_SetPLL1(15);

	emc_WaitMS(30);

	CGU_UpdateClock();

	emc_WaitMS(10);

	__enable_irq();

	return 0;
}