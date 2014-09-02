// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <variant.h>
#include <lpc18xx_gpio.h>
#include "hw.h"

#define GPIO_INPUT 0x00
#define GPIO_OUTPUT 0x01

#define PIN_MODE (FILTER_ENABLE | INBUF_ENABLE)

// Pin modes for use in GPIO only (other pin functions set their own mode)
uint8_t pin_modes[NUM_PINS] = {0};

void hw_digital_configure_gpio(uint8_t ulPin) {
	scu_pinmux(g_APinDescription[ulPin].port,
		g_APinDescription[ulPin].pin,
		PIN_MODE | pin_modes[ulPin],
		g_APinDescription[ulPin].func);
}

int hw_digital_set_mode (uint8_t ulPin, uint8_t mode)
{
	// JS only gets to override pull resistor bits
	pin_modes[ulPin] = mode & (PUP_DISABLE | PDN_ENABLE);
	hw_digital_configure_gpio(ulPin);
	return 0;
}

uint8_t hw_digital_get_mode (uint8_t ulPin)
{
	return pin_modes[ulPin];
}

void hw_digital_output (uint8_t ulPin)
{
	// if (g_APinDescription[ulPin].portNum == NO_PORT
	// 	|| g_APinDescription[ulPin].bitNum == NO_BIT
	// 	|| g_APinDescription[ulPin].func == NO_FUNC) {
	// 	return;
	// }

	hw_digital_configure_gpio(ulPin);

	// set the direction
	GPIO_SetDir(g_APinDescription[ulPin].portNum,
		1 << (g_APinDescription[ulPin].bitNum),
		GPIO_OUTPUT);
}

void hw_digital_startup (uint8_t ulPin) {
	pin_modes[ulPin] = PUP_ENABLE | PDN_DISABLE;
	hw_digital_configure_gpio(ulPin);

	// set the direction
	GPIO_SetDir(g_APinDescription[ulPin].portNum,
		1 << (g_APinDescription[ulPin].bitNum),
		GPIO_INPUT);
}

void hw_digital_input (uint8_t ulPin)
{
	// if (g_APinDescription[ulPin].portNum == NO_PORT
	// 	|| g_APinDescription[ulPin].func == NO_FUNC) {
	// 	return;
	// }

	hw_digital_configure_gpio(ulPin);

	// set the direction
	GPIO_SetDir(g_APinDescription[ulPin].portNum,
		1 << (g_APinDescription[ulPin].bitNum),
		GPIO_INPUT);
}


void hw_digital_write (size_t ulPin, uint8_t ulVal)
{
	if (ulVal != HW_LOW) {
		GPIO_SetValue(g_APinDescription[ulPin].portNum, 1 << g_APinDescription[ulPin].bitNum);
	} else {
		GPIO_ClearValue(g_APinDescription[ulPin].portNum, 1 << (g_APinDescription[ulPin].bitNum));
	}
}


uint8_t hw_digital_read ( size_t ulPin )
{
	uint32_t res = GPIO_ReadValue(g_APinDescription[ulPin].portNum);
	return (res & (1<<g_APinDescription[ulPin].bitNum)) != 0 ? 1 : 0;
}

void hw_interrupt_enable(int index, int ulPin, int mode)
{
	assert(index >= 0 && index < 8);

	unsigned portNum = g_APinDescription[ulPin].portNum;
	unsigned bitNum = g_APinDescription[ulPin].bitNum;
	unsigned int_id = PIN_INT0_IRQn + index;

	/* Clear IRQ in case it existed already */
	NVIC_DisableIRQ(int_id);

	hw_digital_input(ulPin);

	/* Configure GPIO interrupt */
	GPIO_IntCmd(index, portNum, bitNum, mode);

	// Clear any pre-existing interrupts requests so it doesn't fire immediately
	GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, index);
	GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, index);
	GPIO_ClearInt(TM_INTERRUPT_MODE_HIGH, index);
	GPIO_ClearInt(TM_INTERRUPT_MODE_LOW, index);

	/* Enable interrupt for Pin Interrupt */
	NVIC_EnableIRQ(int_id);
}

void hw_interrupt_disable(int index, int mode) {

	if (mode & TM_INTERRUPT_MASK_BIT_RISING) {
		LPC_GPIO_PIN_INT->IENR &= ~(1<<index);//rising edge
	}
	if (mode & TM_INTERRUPT_MASK_BIT_FALLING) {
		LPC_GPIO_PIN_INT->IENF &= ~(1<<index);//falling edge
	}
	if (mode & TM_INTERRUPT_MASK_BIT_HIGH) {
		LPC_GPIO_PIN_INT->IENR &= ~(1<<index);
		LPC_GPIO_PIN_INT->SIENF &= ~(1<<index);
	}
	if (mode & TM_INTERRUPT_MASK_BIT_LOW) {
		LPC_GPIO_PIN_INT->IENR &= ~(1<<index);
		LPC_GPIO_PIN_INT->CIENF &= ~(1<<index);
	}
}



#ifdef __cplusplus
}
#endif
