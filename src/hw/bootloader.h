#pragma once
#include <stdint.h>

#define BOOT_MAGIC 0xBB5ABBBB

void jump_to_flash(void* addr, uint32_t r0_val);
