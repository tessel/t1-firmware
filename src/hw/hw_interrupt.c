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
		hw_interrupt_disable(i);

		tm_event_unref(&interrupts[i].event);

		// Remove any assignments that may have been left over. 
		interrupts[i].pin = NO_ASSIGNMENT;
		interrupts[i].mode = NO_ASSIGNMENT;
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

int hw_interrupt_unwatch(int interrupt_index) {
	// If the interrupt ID was valid
	if (interrupt_index >= 0 && interrupt_index < NUM_INTERRUPTS) {
		// Detatch it so it's not called anymore
		hw_interrupt_disable(interrupt_index);

		// Indicate in data structure that it's a free spot
		interrupts[interrupt_index].pin = NO_ASSIGNMENT;
		interrupts[interrupt_index].mode = NO_ASSIGNMENT;
		interrupts[interrupt_index].callback = NULL;

		tm_event_unref(&interrupts[interrupt_index].event);

		return 1;
	}
	else {
		return NO_ASSIGNMENT;
	}
}

int hw_interrupt_watch (int pin, int mode, int interrupt_index, void (*callback)())
{
	if (!hw_valid_pin(pin)) {
		return -1;
	}

	// If the interrupt ID was valid
	if (interrupt_index >= 0 && interrupt_index < NUM_INTERRUPTS) {

		// Assign the pin to the interrupt
		interrupts[interrupt_index].pin = pin;
		interrupts[interrupt_index].callback = callback;

		// If this is a rising interrupt
		if (mode == TM_INTERRUPT_MODE_RISING) {
			// Attach the rising interrupt to the pin
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_RISING;
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_RISING);
		}
		// If this is a falling interrupt
		else if (mode == TM_INTERRUPT_MODE_FALLING) {
			// Attach the falling interrupt to the pin
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_FALLING;
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_FALLING);
		}
		else if (mode == TM_INTERRUPT_MODE_HIGH) {
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_HIGH;
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_HIGH);
		}
		else if (mode == TM_INTERRUPT_MODE_LOW) {
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_LOW;
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_LOW);
		}
		else if (mode == TM_INTERRUPT_MODE_CHANGE) {
			interrupts[interrupt_index].mode = TM_INTERRUPT_MODE_CHANGE;
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_RISING);
			hw_interrupt_enable(interrupt_index, pin, TM_INTERRUPT_MODE_FALLING);
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
	lua_pushnumber(L, interrupt->mode);
	lua_pushnumber(L, interrupt->state);
	tm_checked_call(L, 4);
}



void place_awaiting_interrupt(int interrupt_id)
{
	GPIO_Interrupt* interrupt = &interrupts[interrupt_id];

	if (interrupt->mode == TM_INTERRUPT_MODE_LOW) {
		LPC_GPIO_PIN_INT->CIENR |= (1<<interrupt_id);
		GPIO_ClearInt(TM_INTERRUPT_MODE_LOW, interrupt_id);
		hw_interrupt_disable(interrupt_id);
	}
	else if (interrupt->mode == TM_INTERRUPT_MODE_HIGH){
		LPC_GPIO_PIN_INT->CIENR |= (1<<interrupt_id);
		GPIO_ClearInt(TM_INTERRUPT_MODE_HIGH, interrupt_id);
		hw_interrupt_disable(interrupt_id);
	}

	else if ((interrupt->mode == TM_INTERRUPT_MODE_RISING) ||
		 (interrupt->mode == TM_INTERRUPT_MODE_CHANGE)) {

		GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, interrupt_id);
		interrupt->state = 1;
	}
	// If we're falling 
	else if ((interrupt->mode == TM_INTERRUPT_MODE_FALLING) ||
		 (interrupt->mode == TM_INTERRUPT_MODE_CHANGE)) {

		GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, interrupt_id);
		interrupt->state = 0;
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
