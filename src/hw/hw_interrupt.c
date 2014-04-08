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
#include "tessel.h"

#include "lpc18xx_gpio.h"


/**
 * Callbacks
 */

void (* gpio0_callback)(int, int, int);
void (* gpio2_callback)(int, int, int);

#define MAX_INT 5
#define RISING_FLAG 0
#define RISING_BIT 1
#define FALLING_FLAG 1
#define FALLING_BIT 2

// Array of interrupt IDs
static const uint8_t GPIO_INT[] = {2, 4, 5, 6, 7};

// Initialize the array of interrupt assignments
// Each assignment index corresponds to an index in the GPIO_INT array
// ie pin at index 0 in assignments will be using interrupt 2 from GPIO_INT
static int assignments[MAX_INT] = {-1, -1, -1, -1, -1};

static uint8_t int_states[MAX_INT] = {0, 0, 0, 0, 0};

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

	/* Enable interrupt for Pin Interrupt */
	NVIC_EnableIRQ(INT_ID);
}

static void detatchGPIOInterruptN (uint8_t INT_ID) {

	// Not sure if there is anything else I need to do at the moment...
	NVIC_DisableIRQ(INT_ID);
}

// When push is cancelled and board is reset, reset all interrupts and num available
void initialize_GPIO_interrupts() {
	for (int i = 0; i < MAX_INT; i++) {
		// Detatch any interrupts
		detatchGPIOInterruptN(GPIO_INT[i]);

		// Remove any assignments that may have been left over. 
		assignments[i] = -1;
	}

}

int hw_interrupts_available (void)
{
	int available = 0;
	// Grab the next available interrupt index
	for (int i = 0; i < MAX_INT; i++) {
		if (assignments[i] == -1) {
			// Return the corresponding ID
			available++;
		}
	}

	return available;
}

int hw_interrupt_index_helper (int interruptID)
{
	// Iterate through possible interrupts
	for (int i = 0; i < MAX_INT; i++) {

		// If an assignment equals the query
		if (GPIO_INT[i] == interruptID) {

			// Return the index
			return i;
		}
	}

	// If there are no matches, return failure code
	return -1;
}

// Check which pin an interrupt is assigned to (-1 if none)
int hw_interrupt_assignment_query (int pin)
{
	// Interate through pin assignments
	for (int i = 0; i < MAX_INT; i++) {

		// If this assignment is for this pin
		if (assignments[i] == pin) {

			// Return the corressponding interrupt id
			return GPIO_INT[i];
		}
	}

	// Return error code if none found
	return -1;
}

int hw_interrupt_acquire (void)
{
	int interruptID = -1;

	// If there are interrupts available
	if (hw_interrupts_available()) {

		// Grab the next available interrupt index
		for (int i = 0; i < MAX_INT; i++) {
			if (assignments[i] == -1) {
				// Return the corresponding ID
				return GPIO_INT[i];
			}
		}
	}
	return interruptID;
}

int hw_interrupt_watch (int ulPin, int flag, int interruptID)
{
	// Grab the index in the array for this interrupt
	int index = hw_interrupt_index_helper(interruptID);

	// If the interrupt ID was valid
	if (index != -1) {

		// If we are turning on an interrupt
		if (flag) {
			// Assign the pin to the interrupt
			assignments[index] = ulPin;

			// If this is a rising interrupt
			if (flag & RISING_BIT) {
				// Attach the rising interrupt to the pin
				hw_interrupt_listen(interruptID, ulPin, RISING_FLAG);
				// attachGPIOInterruptN(g_APinDescription[ulPin].portNum, g_APinDescription[ulPin].bitNum, g_APinDescription[ulPin].port, g_APinDescription[ulPin].pin, RISING_FLAG, interruptID, PIN_INT0_IRQn + interruptID);
			}
			// If this is a falling interrupt
			else if (flag & FALLING_BIT) {
				// Attach the falling interrupt to the pin
				hw_interrupt_listen(interruptID, ulPin, FALLING_FLAG);
				// attachGPIOInterruptN(g_APinDescription[ulPin].portNum, g_APinDescription[ulPin].bitNum, g_APinDescription[ulPin].port, g_APinDescription[ulPin].pin, FALLING_FLAG, interruptID, PIN_INT0_IRQn + interruptID);
			}
		}
		// If we are cancelling an interrupt
		else {

			// Detatch it so it's not called anymore
			detatchGPIOInterruptN(interruptID);

			// Indicate in data structure that it's a free spot
			assignments[index] = -1;

		}

		// Return success
		return 1;
	}

	// Return failure
	return index;
}

// TODO: Take the emitting OUT of the interrupt handler!!!
void emit_interrupt_event(int interrupt_id, int rise, int fall)
{
	// TODO use these
	(void) rise;
	(void) fall;

	// This might be a really dumb way to do this...
	char emitMessage[100];
	strcpy(emitMessage, "{\"pin\":");
	char end = strlen(emitMessage);
	sprintf(emitMessage+end, "\"%d\",\"interruptID\":", assignments[hw_interrupt_index_helper(interrupt_id)]);
	end = strlen(emitMessage);
	sprintf(emitMessage+end, "\"%d\",\"mode\":", interrupt_id);
	end = strlen(emitMessage);

	// If it was a rising interrupt
	uint8_t *state = &(int_states[(hw_interrupt_index_helper(interrupt_id))]);
	if (*state & (1 << RISING_FLAG)) {
		// Prepare the script message
		strcpy(emitMessage + end, "\"rise\"}");
		script_msg_queue("interrupt", emitMessage, strlen(emitMessage));
	}

	// Else if it was a falling interrupt
	if (*state & (1 << FALLING_FLAG)) {
		strcpy(emitMessage + end, "\"fall\"}");
		// Prepare the script message
		script_msg_queue("interrupt", emitMessage, strlen(emitMessage));
	}
	// Clear the state
	*state = 0;
}



void place_awaiting_interrupt(int interrupt_id) {

	uint8_t *int_status = &(int_states[(hw_interrupt_index_helper(interrupt_id))]);
	// If we're rising
	if (((LPC_GPIO_PIN_INT->RISE)>>interrupt_id) & 0x1) {
		// Set the falling flag
		*int_status |= RISING_BIT;
	}

	// If we're falling
	if ((((LPC_GPIO_PIN_INT->FALL)>>interrupt_id) & 0x1)) {
		// Set the falling flag
		*int_status |= FALLING_BIT;
	}

	enqueue_system_event((void (*)(unsigned)) emit_interrupt_event, interrupt_id);

	// Clear the interrupt
	GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, interrupt_id);
	GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, interrupt_id);
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
