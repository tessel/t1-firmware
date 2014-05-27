// MIT

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

void hw_digital_set_mode (uint8_t ulPin, uint8_t mode)
{
	g_APinDescription[ulPin].mode = mode;

}

uint8_t hw_digital_get_mode (uint8_t ulPin)
{
	return g_APinDescription[ulPin].mode;
}

void hw_digital_output (uint8_t ulPin)
{
	// if (g_APinDescription[ulPin].portNum == NO_PORT
	// 	|| g_APinDescription[ulPin].bitNum == NO_BIT
	// 	|| g_APinDescription[ulPin].func == NO_FUNC) {
	// 	return;
	// }

	scu_pinmux(g_APinDescription[ulPin].port,
		g_APinDescription[ulPin].pin,
		g_APinDescription[ulPin].mode,
		g_APinDescription[ulPin].func);

	// set the direction
	GPIO_SetDir(g_APinDescription[ulPin].portNum,
		1 << (g_APinDescription[ulPin].bitNum),
		GPIO_OUTPUT);
}

void hw_digital_startup (uint8_t ulPin) {
	scu_pinmux(g_APinDescription[ulPin].port,
		g_APinDescription[ulPin].pin,
		PUP_ENABLE | PDN_DISABLE | FILTER_ENABLE,
		g_APinDescription[ulPin].func);

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

	scu_pinmux(g_APinDescription[ulPin].port,
		g_APinDescription[ulPin].pin,
		g_APinDescription[ulPin].mode,
		g_APinDescription[ulPin].func);

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

void hw_interrupt_disable(int index) {
	NVIC_DisableIRQ(PIN_INT0_IRQn + index);
}



#ifdef __cplusplus
}
#endif
