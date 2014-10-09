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
#include "hw.h"
#include "variant.h"
#include "spi_flash.h"
#include "bootloader.h"

#include <string.h>

#define TESSEL_BOARD_V 3

#define OTP_BANK3_MASK (0x0F << 28)
#define TESSEL_BOARD_OTP(V) (V << 28)

/*--------------
 * OTP programming
 *--------------*/

#include "otp.h"
#include "otprom.h"

void OTP_fix(volatile unsigned dummy0,volatile unsigned dummy1,volatile unsigned
dummy2,volatile unsigned dummy3)
{
	(void) dummy0;
	(void) dummy1;
	(void) dummy2;
	(void) dummy3;
}

int OTP_Program (bootSrc_e boot_mode) {
	volatile uint32_t rval = otp_Init();
	OTP_fix(0,0,0,0);
	rval = otp_ProgBootSrc(boot_mode);
	(void) rval;
	return (*((uint32_t *) 0x40045030) >> 25);
}

void otp_program_boot_spifi ()
{
	hw_digital_write(LED1, 1);
	volatile int ret = OTP_Program(OTP_BOOTSRC_SPIFI);
	if (ret == 2) {
		hw_digital_write(LED2, 1);
	}
	hw_digital_write(LED1, 1);
}

void otp_program_boot_pins ()
{
  hw_digital_write(CC3K_ERR_LED, 1);
  volatile int ret = OTP_Program(OTP_BOOTSRC_PINS);
  if (ret == 0) {
    hw_digital_write(CC3K_CONN_LED, 1);
  }
  hw_digital_write(CC3K_ERR_LED, 1);
}

void otp_program_board_version()
{
  hw_digital_write(LED1, 1);
  volatile uint32_t rval = otp_Init();
  OTP_fix(0,0,0,0);
  rval = otp_ProgGP0(TESSEL_BOARD_OTP(TESSEL_BOARD_V), OTP_BANK3_MASK);
  if (rval == 0) {
    hw_digital_write(LED2, 1);
  }
  hw_digital_write(LED1, 1);
}

int otp_board_version() {
  return (*((uint32_t *) 0x4004503C) >> 28);
}

void SysTick_Handler (void)
{
}

extern unsigned tessel_boot_bin_len;
extern uint8_t tessel_boot_bin[];

int main() {
	__disable_irq();
	SystemInit();
	CGU_Init();

	hw_digital_output(LED1);
	hw_digital_output(LED2);
	hw_digital_output(CC3K_CONN_LED);
	hw_digital_output(CC3K_ERR_LED);

	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 0);
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);

	SCnSCB->ACTLR &= ~2;
	spiflash_init();
	spiflash_erase_bulk();
	// spiflash_erase_sector(0);

	hw_digital_write(LED1, 1);

	for (unsigned i=0; i<tessel_boot_bin_len; i+=FLASH_PAGE_SIZE){
		spiflash_write_page(i, tessel_boot_bin+i, FLASH_PAGE_SIZE);
	}

	spiflash_mem_mode();

	for (unsigned i=0; i<tessel_boot_bin_len; i+=1) {
		if (FLASH_BOOT_ADDR[i] != tessel_boot_bin[i]) {
			hw_digital_write(CC3K_ERR_LED, 1);
			while(1) {}
		}
	}

	hw_digital_write(LED2, 1);

	/*
	if (otp_board_version() == 0) {
		otp_program_boot_spifi();
		otp_program_board_version();
	}
	*/

	SCnSCB->ACTLR |= 2;

	jump_to_flash(FLASH_BOOT_ADDR, BOOT_MAGIC);
}

void check_failed(uint8_t *file, uint32_t line) {
	(void) file;
	(void) line;
  /* User can add his own implementation to report the file name and line number,
   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while(1);
}
