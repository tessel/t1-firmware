/*
 * tm_interrupt.c
 *
 *  Created on: Aug 14, 2013
 *      Author: tim
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hw.h"
#include "tessel.h"
#include "assert.h"
#include "lpc18xx_gpio.h"

/**
 * Callbacks
 */

void (* gpio0_callback)(int, int, int);
void (* gpio2_callback)(int, int, int);

#define MAX_INT 5
#define NO_ASSIGNMENT -1

typedef struct {
	int interrupt_id;
	int pin;
	int mode;
	int state;
} GPIO_Interrupt;

GPIO_Interrupt interrupts[] = {	
																{2, NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT},
																{4, NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT},
																{5, NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT},
																{6, NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT},
																{7, NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT}
															};

uint32_t hw_uptime_micro ();

static void attachGPIOInterruptN (uint8_t PIN_INT_PORT, uint8_t PIN_INT_BIT, uint8_t PIN_INT_MUX_PORT, uint8_t PIN_INT_MUX_PIN, uint8_t PIN_INT_MODE, uint8_t PIN_INT_NUM, uint8_t INT_ID)
{
	/* Clear IRQ in case it existed already */
	NVIC_DisableIRQ(INT_ID);

	/* Configure pin function */
	scu_pinmux(PIN_INT_MUX_PORT, PIN_INT_MUX_PIN, (MD_PLN|MD_EZI), FUNC0);

	/* Set direction for GPIO port */
	GPIO_SetDir(PIN_INT_PORT,(1<<PIN_INT_BIT), 0);

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(INT_ID, ((0x02<<3)|0x02));

	/* Configure GPIO interrupt */
	GPIO_IntCmd(PIN_INT_NUM,PIN_INT_PORT,PIN_INT_BIT, PIN_INT_MODE);

	// Clear any pre-existing interrupts requests so it doesn't fire immediately
	GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, PIN_INT_NUM);
	GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, PIN_INT_NUM);
	GPIO_ClearInt(TM_INTERRUPT_MODE_HIGH, PIN_INT_NUM);
	GPIO_ClearInt(TM_INTERRUPT_MODE_LOW, PIN_INT_NUM);

	/* Enable interrupt for Pin Interrupt */
	NVIC_EnableIRQ(INT_ID);
}

static void detatchGPIOInterruptN (uint8_t interrupt_id) {

	// Not sure if there is anything else I need to do at the moment...
	NVIC_DisableIRQ(PIN_INT0_IRQn + interrupt_id);
}

// When push is cancelled and board is reset, reset all interrupts and num available
void initialize_GPIO_interrupts() {

	for (int i = 0; i < MAX_INT; i++) {
		// Detatch any interrupts
		detatchGPIOInterruptN(interrupts[i].interrupt_id);

		// Remove any assignments that may have been left over. 
		interrupts[i].pin = NO_ASSIGNMENT;
		interrupts[i].mode = NO_ASSIGNMENT;
	}

}

int hw_interrupts_available (void)
{
	int available = 0;
	// Grab the next available interrupt index
	for (int i = 0; i < MAX_INT; i++) {
		// If this pin isn't assigned
		if (interrupts[i].pin == NO_ASSIGNMENT) {
			// Increment num available
			available++;
		}
	}

	return available;
}

int hw_interrupt_index_helper (int interrupt_id)
{
	// Iterate through possible interrupts
	for (int i = 0; i < MAX_INT; i++) {

		// If an assignment equals the query
		if (interrupts[i].interrupt_id == interrupt_id) {

			// Return the index
			return i;
		}
	}

	// If there are no matches, return failure code
	return NO_ASSIGNMENT;
}

// Check which pin an interrupt is assigned to (-1 if none)
int hw_interrupt_assignment_query (int pin)
{
	// Interate through pin assignments
	for (int i = 0; i < MAX_INT; i++) {

		// If this assignment is for this pin
		if (interrupts[i].pin == pin) {

			// Return the corressponding interrupt id index
			return i;
		}
	}

	// Return error code if none found
	return NO_ASSIGNMENT;
}

int hw_interrupt_acquire (void)
{
	// If there are interrupts available
	if (hw_interrupts_available()) {

		// Grab the next available interrupt index
		for (int i = 0; i < MAX_INT; i++) {

			// If the pin is not assisgned, we'll use this one
			if (interrupts[i].pin == NO_ASSIGNMENT) {

				// Return the corresponding ID
				return i;
			}
		}
	}
	return NO_ASSIGNMENT;
}

int hw_interrupt_unwatch(int interrupt_index) {

	// If the interrupt ID was valid
	if (interrupt_index != NO_ASSIGNMENT) {
		// Detatch it so it's not called anymore
		detatchGPIOInterruptN(interrupts[interrupt_index].interrupt_id);

		// Indicate in data structure that it's a free spot
		interrupts[interrupt_index].pin = NO_ASSIGNMENT;
		interrupts[interrupt_index].mode = NO_ASSIGNMENT;
		return 1;
	}
	else {
		return NO_ASSIGNMENT;
	}
}

int hw_interrupt_watch (int pin, int mode, int interrupt_index)
{
	// If the interrupt ID was valid
	if (interrupt_index != NO_ASSIGNMENT) {

		// Assign the pin to the interrupt
		interrupts[interrupt_index].pin = pin;
		// If this is a rising interrupt
		if (mode == TM_INTERRUPT_MODE_RISING) {
			// Attach the rising interrupt to the pin
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_RISING;
			hw_interrupt_listen(interrupts[interrupt_index].interrupt_id, pin, TM_INTERRUPT_MODE_RISING);
		}
		// If this is a falling interrupt
		else if (mode == TM_INTERRUPT_MODE_FALLING) {
			// Attach the falling interrupt to the pin
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_FALLING;
			hw_interrupt_listen(interrupts[interrupt_index].interrupt_id, pin, TM_INTERRUPT_MODE_FALLING);
		}
		else if (mode == TM_INTERRUPT_MODE_HIGH) {
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_HIGH;
			hw_interrupt_listen(interrupts[interrupt_index].interrupt_id, pin, TM_INTERRUPT_MODE_HIGH);
		}
		else if (mode == TM_INTERRUPT_MODE_LOW) {
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_LOW;
			hw_interrupt_listen(interrupts[interrupt_index].interrupt_id, pin, TM_INTERRUPT_MODE_LOW);
		}
		else if (mode == TM_INTERRUPT_MODE_CHANGE) {
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_CHANGE;
			hw_interrupt_listen(interrupts[interrupt_index].interrupt_id, pin, TM_INTERRUPT_MODE_RISING);
			hw_interrupt_listen(interrupts[interrupt_index].interrupt_id, pin, TM_INTERRUPT_MODE_FALLING);
		}
		// Return success
		return 1;
	}

	// Return failure
	return NO_ASSIGNMENT;
}


void emit_interrupt_event(int interrupt_id, int rise, int fall)
{
	// TODO use these
	(void) rise;
	(void) fall;
	int interrupt_index = hw_interrupt_index_helper(interrupt_id);
	GPIO_Interrupt interrupt = interrupts[interrupt_index];

	char emitMessage[100];

	sprintf(emitMessage, "{\"pin\":\"%d\",", interrupt.pin);
	char end = strlen(emitMessage);

	sprintf(emitMessage+end, "\"interrupt\":\"%d\",", interrupt_index);
	end = strlen(emitMessage);

	sprintf(emitMessage+end, "\"mode\":\"%d\",", interrupt.mode);
	end = strlen(emitMessage);

	sprintf(emitMessage+end, "\"state\": \"%d\",", interrupt.state);
	end = strlen(emitMessage);

	// Can't return time until we refactor CC3k Interrupt code
	sprintf(emitMessage+end, "\"time\": \"%d\"", 0);//(int)hw_uptime_micro());
	end = strlen(emitMessage);

	sprintf(emitMessage+end, "}");

	script_msg_queue("interrupt", emitMessage, strlen(emitMessage));
}



void place_awaiting_interrupt(int interrupt_id) {

	GPIO_Interrupt interrupt = interrupts[hw_interrupt_index_helper(interrupt_id)];

	if (interrupt.mode == TM_INTERRUPT_MODE_LOW) {
		GPIO_ClearInt(TM_INTERRUPT_MODE_LOW, interrupt_id);
		LPC_GPIO_PIN_INT->CIENF |= (1<<interrupt_id);
		detatchGPIOInterruptN(interrupt_id);
	}
	else if (interrupt.mode == TM_INTERRUPT_MODE_HIGH){

		GPIO_ClearInt(TM_INTERRUPT_MODE_HIGH, interrupt_id);
		LPC_GPIO_PIN_INT->SIENF |= (1<<interrupt_id);
		detatchGPIOInterruptN(interrupt_id);
	}

	else if ((interrupt.mode == TM_INTERRUPT_MODE_RISING) ||
		 (interrupt.mode == TM_INTERRUPT_MODE_CHANGE)) {

		GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, interrupt_id);
		interrupt.state = 1;
	}
	// If we're falling 
	else if ((interrupt.mode == TM_INTERRUPT_MODE_FALLING) ||
		 (interrupt.mode == TM_INTERRUPT_MODE_CHANGE)) {

		GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, interrupt_id);
		interrupt.state = 0;
	} 

	enqueue_system_event((void (*)(unsigned)) emit_interrupt_event, interrupt_id);
}

void __attribute__ ((interrupt)) GPIO2_IRQHandler(void)
{
	uint8_t interrupt_id = 2;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}

void __attribute__ ((interrupt)) GPIO4_IRQHandler(void)
{
	uint8_t interrupt_id = 4;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}
void __attribute__ ((interrupt)) GPIO5_IRQHandler(void)
{	
	uint8_t interrupt_id = 5;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}
void __attribute__ ((interrupt)) GPIO6_IRQHandler(void)
{
	uint8_t interrupt_id = 6;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}
void __attribute__ ((interrupt)) GPIO7_IRQHandler(void)
{
	uint8_t interrupt_id = 7;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}

// SHIT IT'S THE OLD INTERNAL INTERRUPT STUFF...

void hw_interrupt_listen (int index, int ulPin, int mode)
{
		assert(index >= 0 && index < 8);
		attachGPIOInterruptN(g_APinDescription[ulPin].portNum, g_APinDescription[ulPin].bitNum, g_APinDescription[ulPin].port, g_APinDescription[ulPin].pin, mode, index, PIN_INT0_IRQn + index);
}


void hw_interrupt_callback_attach (int n, void (*callback)(int, int))
{
	if (n == 2) {
		gpio2_callback = (volatile void*) callback;
	} else {
		gpio0_callback = (volatile void*) callback;
	}
}

void hw_interrupt_callback_detach (int n)
{
	(void) n; // TODO
	gpio0_callback = NULL;
}
