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
