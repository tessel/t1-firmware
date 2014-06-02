// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#pragma once
#include "usb_lpc18_43.h"
#include "class/dfu/dfu.h"

#define DFU_INTF 0

#define DFU_DEST_BASE ((uint8_t*)0x10000000)
#define DFU_TRANSFER_SIZE 4096
#define RAM_DEST_SIZE 96*1024*1024

void usb_init();

typedef enum {
	TARGET_FLASH = 0,
	TARGET_RAM = 1,
} DFU_Target;

extern DFU_Target dfu_target;
