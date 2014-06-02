// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#include <LPC18xx.h>
#include <core_cm3.h>
#include "linker.h"
#include "lpc18xx_ssp.h"
#include "lpc18xx_scu.h"
#include "spi_flash.h"
#include "lpc18xx_cgu.h"

#include <stdbool.h>

typedef struct
{
	volatile uint32_t CTRL;
	volatile uint32_t CMD;
	volatile uint32_t ADDR;
	volatile uint32_t DATINTM;
	volatile uint32_t CACHELIMIT;
	union {
		volatile uint32_t DAT;
		volatile uint8_t DAT8;
	};
	volatile uint32_t MEMCMD;
	volatile uint32_t STAT;
} LPC_SPIFI_TypeDef;

#define SPIFI_STATUS_MCINIT (1<<0)
#define SPIFI_STATUS_CMD    (1<<1)
#define SPIFI_STATUS_RESET  (1<<4)

#define SPIFI_FRAMEFORM_0 (0x1 << 21)
#define SPIFI_FRAMEFORM_1 (0x2 << 21)
#define SPIFI_FRAMEFORM_2 (0x3 << 21)
#define SPIFI_FRAMEFORM_3 (0x4 << 21)
#define SPIFI_FRAMEFORM_4 (0x5 << 21)
#define SPIFI_FRAMEFORM_ADDR3_ONLY (0x6 << 21)

#define SPIFI_FIELDFORM_ALL_SERIAL (0 << 19)
#define SPIFI_FIELDFORM_QUAD_DATA  (1 << 19)
#define SPIFI_FIELDFORM_SERIAL_OPCODE (2 << 19)

#define SPIFI_DATALEN_MASK 0x3fff

#define SPIFI_CR1_QUAD (1<<1)
#define SPIFI_CR1_LC0  (1<<6)
#define SPIFI_CR1_LC1  (1<<7)

LPC_SPIFI_TypeDef * LPC_SPIFI = (LPC_SPIFI_TypeDef*)0x40003000;

/// Configure the SPIFI pins
void spifi_pinmux() {
	// spi pins
	// MISO: P3_6 (spi: 5, spifi: 3)
	LPC_SCU->SFSP3_6 = FUNC3 | SSP_IO;
	// MOSI: P3_7 (spi: 5, spifi: 3)
	LPC_SCU->SFSP3_7 = FUNC3 | SSP_IO;
	// IO2: P3_5
	LPC_SCU->SFSP3_5 = FUNC3 | SSP_IO;
	// IO3: P3_4
	LPC_SCU->SFSP3_4 = FUNC3 | SSP_IO;
	// SCK:  P3_3 (spi: 2, spifi: 3)
	LPC_SCU->SFSP3_3 = FUNC3 | SSP_IO;
	// CS:   P3_8 (gpio: 4 - GPIO5[11], spifi: 3)
	LPC_SCU->SFSP3_8 = FUNC3 | SSP_IO;
}


/// Reset into cmd mode
_ramfunc void spifi_reset() {
	CGU_SetDIV(CGU_CLKSRC_IDIVA, 4); // Slow SPIFI clock
	LPC_SPIFI->STAT = SPIFI_STATUS_RESET;
	while(LPC_SPIFI->STAT & SPIFI_STATUS_RESET);
}

/// Execute a SPI command
_ramfunc static inline void spifi_cmd(uint8_t opcode, unsigned frameform, unsigned addr, bool dir_out, unsigned datalen, uint8_t* buffer) {
	LPC_SPIFI->ADDR = addr;
	LPC_SPIFI->CMD = (opcode << 24) | frameform | SPIFI_FIELDFORM_ALL_SERIAL | (dir_out << 15) | (datalen & SPIFI_DATALEN_MASK);

	for (unsigned int i=0; i<datalen; i++) {
		if (dir_out) {
			LPC_SPIFI->DAT8 = buffer[i];
		} else {
			buffer[i] = LPC_SPIFI->DAT8;
		}
	}

	while(LPC_SPIFI->STAT & SPIFI_STATUS_CMD);
}

/// Init and test SPIFI in command mode
void spiflash_init() {
	spifi_pinmux();
	spifi_reset();

	volatile uint32_t cfi = spiflash_cfi();
	while(cfi != 0x010219); // Assert CFI chip ID is as expected
}

_ramfunc uint8_t spiflash_status() {
	uint8_t r = 0;
	spifi_cmd(0x05, SPIFI_FRAMEFORM_0, 0, false, 1, &r);
	return r;
}

_ramfunc uint32_t spiflash_cfi() {
	uint8_t buf[3] = {0, 0, 0};
	spifi_cmd(0x9f, SPIFI_FRAMEFORM_0, 0, false, 3, buf);
	return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

_ramfunc void spiflash_write_enable() {
	spifi_cmd(0x06, SPIFI_FRAMEFORM_0, 0, false, 0, NULL);
}

_ramfunc void spiflash_write_disable() {
	spifi_cmd(0x04, SPIFI_FRAMEFORM_0, 0, false, 0, NULL);
}

/// Wait for a write or erase operation to complete
_ramfunc void spiflash_wait() {
	while(spiflash_status() & 0x1);
}

_ramfunc void spiflash_write_page(unsigned addr, uint8_t* data, unsigned length) {
	spiflash_wait();
	spiflash_write_enable();

	spifi_cmd(0x12, SPIFI_FRAMEFORM_4, addr, true, length, data);
}

_ramfunc void spiflash_read(unsigned addr, uint8_t* data, unsigned length) {
	spifi_cmd(0x13, SPIFI_FRAMEFORM_4, addr, false, length, data);
}

_ramfunc void spiflash_erase_sector(unsigned addr) {
	spiflash_wait();
	spiflash_write_enable();

	spifi_cmd(0xDC, SPIFI_FRAMEFORM_4, addr, false, 0, NULL);
}

_ramfunc void spiflash_mem_mode_slow() {
	LPC_SPIFI->MEMCMD = (0x13 << 24) | SPIFI_FRAMEFORM_4 | SPIFI_FIELDFORM_ALL_SERIAL;
	while (!(LPC_SPIFI->STAT & SPIFI_STATUS_MCINIT));
}

_ramfunc void spiflash_mem_mode() {
	// WRR CF1 to enable quad and set latency cycles
	spiflash_wait();
	spiflash_write_enable();
	uint8_t buf[2] = {0, (SPIFI_CR1_QUAD)};
	spifi_cmd(0x01, SPIFI_FRAMEFORM_0, 0, true, 2, buf);

	//                timeout         CS high time   fbclk
	LPC_SPIFI->CTRL = (0xffff << 0) | (0x1 << 16)  | (1 << 30);

	LPC_SPIFI->DATINTM = 0;
	LPC_SPIFI->MEMCMD = (0xEC << 24) | SPIFI_FRAMEFORM_4 | SPIFI_FIELDFORM_SERIAL_OPCODE | ((2 + 1) << 16);
	while (!(LPC_SPIFI->STAT & SPIFI_STATUS_MCINIT));

	// Execute some instructions, or it crashes when jumping to flash...
	volatile unsigned j=100000;
	while (j--);

	CGU_SetDIV(CGU_CLKSRC_IDIVA, 2); // Fast SPIFI clock
}

_ramfunc void spiflash_enter_cmd_mode() {
	__disable_irq();
	SCnSCB->ACTLR &= ~2;
	spifi_reset();
}

_ramfunc void spiflash_leave_cmd_mode() {
	spiflash_mem_mode();

	SCnSCB->ACTLR |= 2;
	__enable_irq();
}

/// Write a buffer to flash. The address must point to the beginning of a flash erase sector.
/// This is meant to be called from SPIFI code
_ramfunc void spiflash_write_buf(unsigned addr, uint8_t* data, unsigned length) {
	spiflash_enter_cmd_mode();

	for (unsigned i=0; i<length; i+=FLASH_SECTOR_SIZE) {
		spiflash_erase_sector(addr + i);
	}

	for (unsigned i=0; i<length; i+=FLASH_PAGE_SIZE){
		spiflash_write_page(addr+i, data+i, MIN(FLASH_PAGE_SIZE, length-i));
	}

	spiflash_leave_cmd_mode();
}

_ramfunc void spiflash_reinit() {
	spiflash_enter_cmd_mode();
	spiflash_leave_cmd_mode();
}
