/*
 * tm_interrupt.c
 *
 *  Created on: Aug 14, 2013
 *      Author: tim
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tm.h"
#include "colony.h"
#include "hw.h"
#include "tessel.h"
#include "assert.h"
#include "lpc18xx_gpio.h"

/**
 * Callbacks
 */

#define NO_ASSIGNMENT -1

typedef struct {
	tm_event event;
	int pin;
	int mode;
	int state;
	void (*callback)();
} GPIO_Interrupt;

static void interrupt_callback(tm_event* event);

GPIO_Interrupt interrupts[] = {	
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
	{TM_EVENT_INIT(interrupt_callback), NO_ASSIGNMENT, NO_ASSIGNMENT, NO_ASSIGNMENT, NULL},
};

// When push is cancelled and board is reset, reset all interrupts and num available
void initialize_GPIO_interrupts() {

	for (int i = 0; i < NUM_INTERRUPTS; i++) {
		// Detatch any interrupts
		hw_interrupt_unwatch(i, (&interrupts[i])->mode);
	}
}

int hw_interrupts_available (void)
{
	int available = 0;
	// Grab the next available interrupt index
	for (int i = 0; i < NUM_INTERRUPTS; i++) {
		// If this pin isn't assigned
		if (interrupts[i].pin == NO_ASSIGNMENT) {
			// Increment num available
			available++;
		}
	}

	return available;
}

// Check which pin an interrupt is assigned to (-1 if none)
int hw_interrupt_assignment_query (int pin)
{
	// Interate through pin assignments
	for (int i = 0; i < NUM_INTERRUPTS; i++) {

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
		for (int i = 0; i < NUM_INTERRUPTS; i++) {

			// If the pin is not assisgned, we'll use this one
			if (interrupts[i].pin == NO_ASSIGNMENT) {

				// Return the corresponding ID
				return i;
			}
		}
	}
	return NO_ASSIGNMENT;
}

// Stop watching specific mode of interrupt
int hw_interrupt_unwatch(int interrupt_index, int bitMask) {

	// If the interrupt ID was valid
	if (interrupt_index >= 0 && interrupt_index < NUM_INTERRUPTS) {

		// Detatch it so it's not called anymore
		hw_interrupt_disable(interrupt_index, bitMask);
		
		interrupts[interrupt_index].mode &=  ~bitMask;

		// If there is no more mode in this pin
		if (!interrupts[interrupt_index].mode) {
			// Disable IRQs on this pin
			NVIC_DisableIRQ(PIN_INT0_IRQn + interrupt_index);
			// Indicate in data structure that it's a free spot
			interrupts[interrupt_index].pin = NO_ASSIGNMENT;
			interrupts[interrupt_index].mode = NO_ASSIGNMENT;
			interrupts[interrupt_index].callback = NULL;

			tm_event_unref(&interrupts[interrupt_index].event);
		}

		return 1;
	}
	else {
		return NO_ASSIGNMENT;
	}
}

int hw_interrupt_watch (int pin, int bitMask, int interrupt_index, void (*callback)())
{
	if (!hw_valid_pin(pin)) {
		return -1;
	}

	// If the interrupt ID was valid
	if (interrupt_index >= 0 && interrupt_index < NUM_INTERRUPTS) {

		// Assign the pin to the interrupt
		interrupts[interrupt_index].pin = pin;
		interrupts[interrupt_index].callback = callback;
		interrupts[interrupt_index].mode = bitMask;
		// If this is a rising interrupt
		if (bitMask & TM_INTERRUPT_MASK_BIT_RISING) {
			// Attach the rising interrupt to the pin
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_RISING);
		}
		// If this is a falling interrupt
		if (bitMask & TM_INTERRUPT_MASK_BIT_FALLING) {
			// Attach the falling interrupt to the pin
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_FALLING);
		}
		if (bitMask & TM_INTERRUPT_MASK_BIT_HIGH) {
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_HIGH);
		}
		if (bitMask & TM_INTERRUPT_MASK_BIT_LOW) {
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_LOW);
		}

		tm_event_ref(&interrupts[interrupt_index].event);

		// Return success
		return 1;
	}

	// Return failure
	return NO_ASSIGNMENT;
}


void interrupt_callback(tm_event* event)
{
	GPIO_Interrupt* interrupt = (GPIO_Interrupt*) event;
	int interrupt_index = interrupt - interrupts;

	lua_State* L = tm_lua_state;
	if (!L) return;

	lua_getglobal(L, "_colony_emit");
	lua_pushstring(L, "interrupt");
	lua_pushnumber(L, interrupt_index);
	lua_pushnumber(L, interrupt->state);
	lua_pushnumber(L, tm_uptime_micro());
	tm_checked_call(L, 4);
}



void place_awaiting_interrupt(int interrupt_id)
{
	GPIO_Interrupt* interrupt = &interrupts[interrupt_id];

	// If it's an edge triggered interrupt
	if (interrupt-> mode & (TM_INTERRUPT_MASK_BIT_RISING | TM_INTERRUPT_MASK_BIT_FALLING)) {

		// It's a rising edge and we were waiting for a rising edge
		// Note that this register will change even if you didn't
		// activate the register
		if ((interrupt->mode & TM_INTERRUPT_MASK_BIT_RISING) &&
		 	(LPC_GPIO_PIN_INT->RISE & (1 << interrupt_id))) {
			// Clear the pending interrupt
			GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, interrupt_id);
			// Set the state to high
			interrupt->state = TM_INTERRUPT_MASK_BIT_RISING;
		}
		// It's a falling edge and we were waiting for a rising edge
		// Note that this register will change even if you didn't
		// activate the register
		else if((interrupt->mode & TM_INTERRUPT_MASK_BIT_FALLING) &&
		 	(LPC_GPIO_PIN_INT->FALL & (1 << interrupt_id))) {
			// Clear the pending interrupt
			GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, interrupt_id);
			// Set the state to low
			interrupt->state = TM_INTERRUPT_MASK_BIT_FALLING;
		}
		else {
			// Something went wrong
			return;
		}
	}
	// It's a level trigger
	else {
		// It's a high level
		if (LPC_GPIO_PIN_INT->IENF & (1 << interrupt_id)) {
			// Disable this level trigger so it doesn't fire again
			LPC_GPIO_PIN_INT->CIENR |= (1<<interrupt_id);
			// Clear the pending interrupt
			GPIO_ClearInt(TM_INTERRUPT_MODE_HIGH, interrupt_id);
			// Disable this mode
			hw_interrupt_disable(interrupt_id, interrupt->mode);
			// Set the state to high
			interrupt->state = TM_INTERRUPT_MASK_BIT_HIGH;
		}
		// It's a low level
		else {
			// Disable this level trigger so it doesn't fire again
			LPC_GPIO_PIN_INT->CIENR |= (1<<interrupt_id);
			// Clear the pending interrupt
			GPIO_ClearInt(TM_INTERRUPT_MODE_LOW, interrupt_id);
			// Disable this mode
			hw_interrupt_disable(interrupt_id, interrupt->mode);
			// Set the state to low
			interrupt->state = TM_INTERRUPT_MASK_BIT_LOW;
		}
	}

	if (interrupt->callback != NULL) {
		(*interrupt->callback)();
	}
	else {
		tm_event_trigger(&interrupt->event);
	}
}

void __attribute__ ((interrupt)) GPIO0_IRQHandler(void)
{
	uint8_t interrupt_id = 0;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}

void __attribute__ ((interrupt)) GPIO1_IRQHandler(void)
{
	uint8_t interrupt_id = 1;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}

void __attribute__ ((interrupt)) GPIO2_IRQHandler(void)
{
	uint8_t interrupt_id = 2;
	if (GPIO_GetIntStatus(interrupt_id))
	{
		place_awaiting_interrupt(interrupt_id);
	}
}

void __attribute__ ((interrupt)) GPIO3_IRQHandler(void)
{
	uint8_t interrupt_id = 3;
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
