// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "lpc18xx_cgu.h"
#include "lpc18xx_scu.h"

#include <string.h>
#include <stdbool.h>

#include "variant.h"
#include "hw.h"
#include "spi_flash.h"
#include "bootloader.h"

#include "boot.h"

volatile bool exit_and_jump = 0;
DFU_Target dfu_target = TARGET_FLASH;

/*** SysTick ***/

volatile uint32_t g_msTicks;

/* SysTick IRQ handler */
void SysTick_Handler(void) {
	g_msTicks++;
}

void delay_ms(unsigned ms) {
	unsigned start = g_msTicks;
	while (g_msTicks - start <= ms) {
		__WFI();
	}
}

void init_systick() {
	if (SysTick_Config(SystemCoreClock / 1000)) {	/* Setup SysTick Timer for 1 msec interrupts  */
		while (1) {}								/* Capture error */
	}
	NVIC_SetPriority(SysTick_IRQn, 0x0);
	g_msTicks = 0;
}

/*** USB / DFU ***/

uint8_t* dfu_cb_dnload_block(uint16_t block_num, uint16_t len) {
	if (len > DFU_TRANSFER_SIZE) {
		dfu_error(DFU_STATUS_errUNKNOWN);
		return NULL;
	}
	if (dfu_target == TARGET_FLASH) {
		if (block_num * DFU_TRANSFER_SIZE > FLASH_FW_SIZE) {
			dfu_error(DFU_STATUS_errADDRESS);
			return NULL;
		}
		return DFU_DEST_BASE;
	} else if (dfu_target == TARGET_RAM) {
		if (block_num * DFU_TRANSFER_SIZE > RAM_DEST_SIZE) {
			dfu_error(DFU_STATUS_errADDRESS);
			return NULL;
		}
		return DFU_DEST_BASE + block_num * DFU_TRANSFER_SIZE;
	} else {
		return NULL;
	}
}

_ramfunc unsigned dfu_cb_dnload_block_completed(uint16_t block_num, uint16_t length) {
	if (dfu_target == TARGET_FLASH) {
		unsigned addr = FLASH_FW_START + block_num * DFU_TRANSFER_SIZE;

		if (length > 0) {
			spiflash_enter_cmd_mode();
			if (addr%FLASH_SECTOR_SIZE == 0) {
				spiflash_erase_sector(addr);
			}

			for (unsigned i=0; i<length; i+=FLASH_PAGE_SIZE){
				spiflash_write_page(addr+i, DFU_DEST_BASE+i, FLASH_PAGE_SIZE);
			}
			spiflash_leave_cmd_mode();
		}
	}
	return 0;
}

void dfu_cb_manifest(void) {
	exit_and_jump = 1;
}

/*** LED ***/

uint8_t led_state = 0;
uint32_t led_next_time = 0;

void led_task() {
	if (g_msTicks > led_next_time) {
		led_next_time += 400;
		hw_digital_write(CC3K_ERR_LED, led_state);
		hw_digital_write(CC3K_CONN_LED, led_state);
		led_state = !led_state;
	}
}

void bootloader_main() {
	SystemInit();
	spiflash_reinit();
	CGU_Init();

	hw_digital_output(LED1);
	hw_digital_output(LED2);
	hw_digital_output(CC3K_CONN_LED);
	hw_digital_output(CC3K_ERR_LED);
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);

	init_systick();

	__enable_irq();

	usb_init();
	usb_set_speed(USB_SPEED_FULL);
	usb_attach();

	while(!exit_and_jump) {
		led_task();
		__WFI(); /* conserve power */
	}

	delay_ms(25);

	usb_detach();

	delay_ms(100);

	if (dfu_target == TARGET_RAM) {
		jump_to_flash(DFU_DEST_BASE, 0);
	}
}

bool flash_valid() {
	return ((unsigned *)FLASH_FW_ADDR)[7] == 0x5A5A5A5A;
}

bool software_triggered() {
	return 0;
}

bool button_pressed() {
	return hw_digital_read(BTN1);
}

int main(unsigned r0) {
	hw_digital_input(BTN1);

	if (r0 == BOOT_MAGIC || !flash_valid() || button_pressed()) {
		bootloader_main();
	}

	jump_to_flash(FLASH_FW_ADDR, 0);
}

void check_failed(uint8_t *file, uint32_t line) {
	(void) file;
	(void) line;
	
  /* User can add his own implementation to report the file name and line number,
   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while(1);
}
