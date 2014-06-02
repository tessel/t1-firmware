// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#pragma once

void spiflash_reinit();
void spiflash_init();
uint8_t spiflash_status();
uint32_t spiflash_cfi();
void spiflash_wait();
void spiflash_write_enable();
void spiflash_write_disable();
void spiflash_write_page(unsigned addr, uint8_t* data, unsigned length);
void spiflash_erase_sector(unsigned addr);
void spiflash_read(unsigned addr, uint8_t* data, unsigned length);
void spiflash_mem_mode();

void spiflash_enter_cmd_mode();
void spiflash_leave_cmd_mode();

void spiflash_write_buf(unsigned addr, uint8_t* data, unsigned length);


#define FLASH_SECTOR_SIZE (64*1024)
#define FLASH_PAGE_SIZE   256

// Flash partitions
// These must be aligned on sector boundaries

#define FLASH_BOOT_START       0
#define FLASH_BOOT_SIZE        (64*1024)

#define FLASH_FW_START         (64*1024)
#define FLASH_FW_SIZE          (2*1024*1024 - 64*1024)

#define FLASH_FS_START        (2*1024*1024)
#define FLASH_FS_SIZE         (30*1024*1024)

#define FLASH_ADDR ((unsigned char*)0x14000000)
#define FLASH_BOOT_ADDR (FLASH_ADDR + FLASH_BOOT_START)
#define FLASH_FW_ADDR   (FLASH_ADDR + FLASH_FW_START)
#define FLASH_FS_MEM_ADDR (FLASH_ADDR + FLASH_FS_START)
