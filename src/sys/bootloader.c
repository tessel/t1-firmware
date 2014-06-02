// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#define CMSIS_BITPOSITIONS
#include "LPC18xx.h"
#include "bootloader.h"

void jump_to_flash(void* addr_p, uint32_t r0_val) {
	uint32_t *addr = addr_p;
	__disable_irq();

	// Disable SysTick
	SysTick->CTRL = 0;

	// Reset peripherals
	LPC_RGU->RESET_CTRL0 =
		( RGU_RESET_CTRL0_USB0_RST_Msk
		| RGU_RESET_CTRL0_DMA_RST_Msk
		| RGU_RESET_CTRL0_SDIO_RST_Msk
		| RGU_RESET_CTRL0_EMC_RST_Msk
		| RGU_RESET_CTRL0_ETHERNET_RST_Msk
		| RGU_RESET_CTRL0_GPIO_RST_Msk
		);

	LPC_RGU->RESET_CTRL1 =
		( RGU_RESET_CTRL1_TIMER0_RST_Msk
		| RGU_RESET_CTRL1_TIMER1_RST_Msk
		| RGU_RESET_CTRL1_TIMER2_RST_Msk
		| RGU_RESET_CTRL1_TIMER3_RST_Msk
		| RGU_RESET_CTRL1_RITIMER_RST_Msk
		| RGU_RESET_CTRL1_SCT_RST_Msk
		| RGU_RESET_CTRL1_MOTOCONPWM_RST_Msk
		| RGU_RESET_CTRL1_QEI_RST_Msk
		| RGU_RESET_CTRL1_ADC0_RST_Msk
		| RGU_RESET_CTRL1_ADC1_RST_Msk
		| RGU_RESET_CTRL1_DAC_RST_Msk
		| RGU_RESET_CTRL1_UART0_RST_Msk
		| RGU_RESET_CTRL1_UART1_RST_Msk
		| RGU_RESET_CTRL1_UART2_RST_Msk
		| RGU_RESET_CTRL1_UART3_RST_Msk
		| RGU_RESET_CTRL1_I2C0_RST_Msk
		| RGU_RESET_CTRL1_I2C1_RST_Msk
		| RGU_RESET_CTRL1_SSP0_RST_Msk
		| RGU_RESET_CTRL1_SSP1_RST_Msk
		| RGU_RESET_CTRL1_I2S_RST_Msk
		| RGU_RESET_CTRL1_CAN1_RST_Msk
		| RGU_RESET_CTRL1_CAN0_RST_Msk
		);

	// Switch to the the interrupt vector table in flash
	SCB->VTOR = (uint32_t) addr;

	// Set up the stack and jump to the reset vector
	uint32_t sp = addr[0];
	uint32_t pc = addr[1];
	register uint32_t r0 __asm__ ("r0") = r0_val;
	__asm__ volatile("mov sp, %0; bx %1" :: "r" (sp), "r" (pc), "r" (r0));
	(void) r0_val;
}
