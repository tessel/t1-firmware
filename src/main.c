// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#define TESSEL_WIFI 1
#define TESSEL_TEST 0

#ifndef COLONY_PRELOAD_ON_INIT
#define COLONY_PRELOAD_ON_INIT 0
#endif


/**
 * Constants
 */



/**
 * Includes
 */

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "tm.h"
#include "hw.h"
#include "tessel.h"
#include "tessel_wifi.h"
#include "l_hw.h"
#include "colony.h"

#include "sdram_init.h"
#include "spi_flash.h"
#include "bootloader.h"
#include "utility/wlan.h"

#include "module_shims/audio-vs1053b.h"

#ifdef TESSEL_TEST
#include "test.h"
#endif


/*----------------------------------------------------------------------------
  Interrupts
 *---------------------------------------------------------------------------*/

void __attribute__ ((interrupt)) GPIO7_IRQHandler(void)
{
	_tessel_cc3000_irq_interrupt();
}

void __attribute__ ((interrupt)) DMA_IRQHandler (void)
{
	_hw_spi_irq_interrupt();
}


/*----------------------------------------------------------------------------
  Main Program
 *---------------------------------------------------------------------------*/

tm_fs_ent* tm_fs_root;

enum {
	SCRIPT_EMPTY,
	SCRIPT_DOWNLOADING,
	SCRIPT_READING
} script_t;

volatile int script_buf_lock = SCRIPT_EMPTY;
uint8_t *script_buf = 0;
size_t script_buf_size = 0;
uint8_t script_buf_flash = false;


/**
 * command interface
 */

void tessel_cmd_process (uint8_t cmd, uint8_t* buf, unsigned size)
{
	if (cmd == 'U' || cmd == 'P') {
		// Info.
		TM_COMMAND('U', "{\"size\": %u}", (unsigned) size);
		TM_DEBUG("");
		TM_DEBUG("Tessel is reading %u bytes...", (unsigned) size);

		if (script_buf_lock == SCRIPT_EMPTY) {
			script_buf_lock = SCRIPT_DOWNLOADING;
		} else {
			return;
		}

		// Copy command into script buffer.
		script_buf_size = size;
		script_buf = buf;
		script_buf_flash = (cmd == 'P');
		script_buf_lock = SCRIPT_READING;
		buf = NULL; // So it won't get freed
	} else if (cmd == 'G') {
		TM_COMMAND('G', "\"pong\"");
	
	} else if (cmd == 'M') {
		if (tm_lua_state != NULL) {
			colony_ipc_emit(tm_lua_state, "raw-message", buf, size);
		}
	
	} else if (cmd == 'B') {
		jump_to_flash(FLASH_BOOT_ADDR, BOOT_MAGIC);
		while(1);
	
	} else if (cmd == 'n') {
		if (tm_lua_state != NULL) {
			colony_ipc_emit(tm_lua_state, "stdin", buf, size);
		}

#if TESSEL_WIFI
	} else if (cmd == 'W') {
		if (size != (32 + 64 + 32)) {
			TM_ERR("WiFi buffer malformed, aborting.");
			TM_COMMAND('W', "{\"event\": \"error\", \"message\": \"Malformed request.\"}");
		} else {
			char wifi_ssid[33] = {0};
			char wifi_pass[65] = {0};
			char wifi_security[33] = {0};

			memcpy(wifi_security, &buf[96], 32);
			memcpy(wifi_ssid, &buf[0], 32);
			memcpy(wifi_pass, &buf[32], 64);
			tessel_wifi_connect(wifi_security, wifi_ssid, wifi_pass);
		}

	} else if (cmd == 'Y') {
		tessel_wifi_disable();

	} else if (cmd == 'C') {
		// check wifi for connection
		tessel_wifi_check(1);

	} else if (cmd == 'V') {
		if (hw_net_is_connected()) {
			char ssid[33] = { 0 };
			hw_net_ssid(ssid);
			tm_logf('V', "Current network: %s", ssid);
		}

		// TODO make this W also?
		uint8_t results[64];
		int res = 0;
		int first = 1;
		while (true) {
			// TODO: this should pass data back as a message in structured form
			// (which requires generating a JSON array)
			res = wlan_ioctl_get_scan_results(0, results);
			char* networkcountptr = (char*) &results[0];
			uint32_t networkcount = *((uint32_t*) networkcountptr);
			if (first) {
				if (networkcount == 0) {
					tm_logf('V', "There are no visible networks yet.");
				} else {
					tm_logf('V', "Currently visible networks (%ld):", networkcount);
					first = 0;
				}
			}
			if (res != 0 || networkcount == 0) {
				break;
			}

			int rssi = 0;
			if (results[8] & 0x1) {
				rssi = results[8] >> 1;
			}

			char* nameptr = (char*) &results[0 + 4 + 4 + 1 + 1 + 2];
			unsigned char namebuf[33] = { 0 };
			memcpy(namebuf, nameptr, 32);
			tm_logf('V', "\t%s (%i/127)", namebuf, rssi);
		}

		if (hw_net_is_connected()) {
			TM_COMMAND('V', "{\"connected\": 1, \"ip\": \"%ld.%ld.%ld.%ld\"}", hw_wifi_ip[0], hw_wifi_ip[1], hw_wifi_ip[2], hw_wifi_ip[3]);
		} else {
			TM_COMMAND('V', "{\"connected\": 0}");
		}

	} else if (cmd == 'D') {
		TM_COMMAND('D', "%d", hw_net_erase_profiles());

#else
	} else if (cmd == 'D' || cmd == 'C' || cmd == 'V' || cmd == 'Y' || cmd == 'W') {
		TM_ERR("WiFi command not enabled on this Tessel.\n");
#endif
	
	} else {
		// Invalid?
		script_buf_lock = SCRIPT_EMPTY;
	}

	free(buf);
}


/**
 * system tick handler
 */

typedef struct tm_anim {
	size_t millis;
	size_t count;
	void (*call)(size_t);
	struct tm_anim * next;
} tm_anim_t;

tm_anim_t* systick_anim_list = NULL;

_ramfunc void SysTick_Handler (void)
{
	tm_anim_t* frame = systick_anim_list;
	while (frame != NULL) {
		frame->count++;
		if (frame->count >= frame->millis) {
			frame->call(frame->count / frame->millis);
		}
		frame = frame->next;
	}
}

void add_animation (tm_anim_t* anim)
{
	anim->next = systick_anim_list;
	systick_anim_list = anim;

	// Interrupt at 1000hz
	NVIC_SetPriority(SysTick_IRQn, ((0x02<<3)|0x01));
	SysTick_Config(CGU_GetPCLKFrequency(CGU_PERIPHERAL_M3CORE) >> 4);
}

tm_anim_t* create_animation (int millis, void (*call)(size_t))
{
	tm_anim_t* cc_anim = (tm_anim_t*) calloc(1, sizeof(tm_anim_t));
	cc_anim->millis = millis >> 6;
	cc_anim->call = call;
	cc_anim->next = NULL;
	return cc_anim;
}

void cc_animation ()
{
	tm_anim_t* anim = create_animation(512, _cc3000_cb_animation_tick);
	add_animation(anim);
}


/**
 * Main body of Tessel OS
 */

void debugstack_hook(lua_State* L, lua_Debug *ar)
{
	(void) ar;
	lua_sethook(L, NULL, 0, 0);

	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return;
	}

	// Call debug.traceback
	lua_call(L, 0, 1);
	lua_remove(L, -2);

	unsigned len = 0;
	uint8_t* trace = (uint8_t*) lua_tolstring(L, -1, &len);
	hw_send_usb_msg('k', trace, len);
}

int debugstack() {
	if (tm_lua_state == 0) {
		return -1;
	}

	if (lua_gethook(tm_lua_state) != 0) {
		// Another hook probably means we're in the process of exiting
		return -2;
	}

	// TODO: what if we're not currently running lua (waiting for event)
	lua_sethook(tm_lua_state, debugstack_hook, LUA_MASKCOUNT, 1);
	return 0;
}


extern char builtin_tessel_js[];
extern unsigned int builtin_tessel_js_len;
void load_script(uint8_t* script_buf, unsigned script_buf_size, uint8_t speculative);

void main_body (void)
{
	// Check for initial load from Flash.
	// TODO this introduces a race condition where Flash set the saved code
	// mutex while code is being loaded.
	if (*((uint32_t *) FLASH_FS_MEM_ADDR) != 0xffffffff) {
		load_script(FLASH_FS_MEM_ADDR, FLASH_FS_SIZE, true);
	}

	while (1) {
		// While we're not running the script, we can do other interrupts.
		TM_DEBUG("Ready.");
		while (script_buf_lock != SCRIPT_READING) {
			hw_wait_for_event();
			tm_event_process();
		}

		if (script_buf_flash) {
			TM_DEBUG("Writing bundle to flash...");
			spiflash_write_buf(FLASH_FS_START, script_buf, script_buf_size);
		}

		load_script(script_buf, script_buf_size, false);
		free(script_buf);
		script_buf_lock = SCRIPT_EMPTY;

		// Retry processing the command now that the script buf is unused
		//tessel_cmd_process(&cmd_usb, hw_usb_cdc_read);
	}
}

void tm_events_lock() { __disable_irq(); }
void tm_events_unlock() { __enable_irq(); }

void hw_wait_for_event() {
	__disable_irq();
	if (!tm_events_pending()) {
		// Check for events after disabling interrupts to avoid the race
		// condition where an event comes from an interrupt between check and
		// sleep. Processor still wakes from sleep on interrupt request even
		// when interrupts are disabled.
		__WFI();
	}
	__enable_irq();
}

void load_script(uint8_t* script_buf, unsigned script_buf_size, uint8_t speculative)
{
	int ret = 0;

	initialize_GPIO_interrupts();

	// Initialize GPDMA
	GPDMA_Init();

	// Reset board state.
	tessel_gpio_init(0);
	tessel_network_reset();

	// Populate filesystem.
	TM_DEBUG("Populating filesystem...");
	tm_fs_root = tm_fs_dir_create_entry();
	ret = tm_fs_mount_tar(tm_fs_root, ".", script_buf, script_buf_size);

	if (ret != 0) {
		if (speculative) {
			return;
		}
		TM_ERR("Error parsing tar file: %d", ret);
		if (ret == -2) {
			TM_ERR("NOTE: Tessel archive expansion does not yet support long file paths.");
			TM_ERR("      You might temporarily resolve the problem by consolidating your");
			TM_ERR("      node_modules folders into a flat, not nested, hierarchy.");
		}
		TM_COMMAND('S', "-126");
		tm_fs_destroy(tm_fs_root);
		tm_fs_root = 0;
		return;
	}

	// Ensure index.js exists.
	tm_fs_t index_fd;
	int start_script_is_indexjs = 0;
	ret = tm_fs_open(&index_fd, tm_fs_root, "/_start.js", 0);
	if (ret != 0) {
		// May be older archive with index.js.
		start_script_is_indexjs = 1;
		ret = tm_fs_open(&index_fd, tm_fs_root, "/index.js", 0);
		if (ret != 0) {
			if (speculative) {
				return;
			}
		
			TM_DEBUG("tm_fs_open error %d\n", ret);
			TM_COMMAND('S', "1");
			TM_ERR("Could not execute code. Ensure that archive sent to Tessel contains data.");
			TM_COMMAND('S', "-127");
			tm_fs_destroy(tm_fs_root);
			tm_fs_root = 0;
			return;
		} else {
			TM_DEBUG("Starting script: /index.js");
			tm_fs_close(&index_fd);
		}
	} else {
		TM_DEBUG("Starting script: /_start.js");
		tm_fs_close(&index_fd);
	}


	// Open runtime.
	TM_DEBUG("Initializing runtime...");
	assert(tm_lua_state == NULL);
	colony_runtime_open();

	lua_State* L = tm_lua_state;

	// Get preload table.
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_remove(L, -2);
	// hw
	lua_pushcfunction(L, luaopen_hw);
	lua_setfield(L, -2, "hw");
	// Done with preload
	lua_pop(L, 1);

	// Open tessel lib.
	TM_DEBUG("Loading tessel library...");
	int res = luaL_loadbuffer(L, builtin_tessel_js, builtin_tessel_js_len, "tessel.js");
	if (res != 0) {
		TM_ERR("Error in %s: %d\n", "tessel.js", res);
		tm_fs_destroy(tm_fs_root);
		tm_fs_root = 0;
		return;
	}
	lua_setglobal(L, "_tessel_lib");

	lua_getglobal(L, "_colony");
	lua_getfield(L, -1, "global");
	lua_getfield(L, -1, "process");
	lua_getfield(L, -1, "versions");
	lua_pushnumber(L, tessel_board_version());
	lua_setfield(L, -2, "tessel_board");

	const char *argv[3];
	argv[0] = "runtime";
	if (start_script_is_indexjs) {
		argv[1] = "index.js";
	} else {
		argv[1] = "_start.js";
	}
	argv[2] = NULL;
	TM_COMMAND('S', "1");
	TM_DEBUG("Running script...");
	TM_DEBUG("Uptime since startup: %fs", ((float) tm_uptime_micro()) / 1000000.0);

	int returncode = tm_runtime_run(argv[1], argv, 2);

	tm_fs_destroy(tm_fs_root);
	tm_fs_root = NULL;

	hw_uart_disable(UART0);
	hw_uart_disable(UART2);
	hw_uart_disable(UART3);

	// Clean up our SPI structs and dereference our lua objects
	hw_spi_async_cleanup();
	// Stop any audio playback/recording and clean up memory
	audio_reset();

	initialize_GPIO_interrupts();
	tessel_gpio_init(0);

	colony_runtime_close();

	TM_COMMAND('S', "%d", -returncode);
	TM_DEBUG("Script ended with return code %d.", returncode);
}


int main (void)
{
	SystemInit();
	spiflash_reinit();
	CGU_Init();
	SDRAM_Init();

	if (tessel_board_version() == 1) {
		g_APinDescription = (PinDescription*) g_APinDescription_boardV0;
	} else if (tessel_board_version() == 2) {
		g_APinDescription = (PinDescription*) g_APinDescription_boardV2;
	} else {
		g_APinDescription = (PinDescription*) g_APinDescription_boardV3;
	}

	tessel_gpio_init(1);

	tm_uptime_init();

	hw_usb_init();

	// Control these outside of tessel_gpio_init()
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);

#if TESSEL_WIFI
	// CC interrupts
	tessel_wifi_init();
#endif

	// Do a light show in 300ms.
	#define LIGHTSHOWDELAY 37500
	hw_digital_write(CC3K_ERR_LED, 1);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(CC3K_CONN_LED, 1);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(LED1, 1);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(LED2, 1);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(LED1, 0);
	hw_wait_us(LIGHTSHOWDELAY);
	hw_digital_write(LED2, 0);
	hw_wait_us(LIGHTSHOWDELAY);

	cc_animation();

#if TESSEL_WIFI && TESSEL_FASTCONNECT
	tessel_wifi_fastconnect();
#endif

#if TESSEL_TEST
	test_hw_spi();
	while (1) { };

	test_gpio_read();
	test_adc();
	test_dac();

	test_i2c('a');

	test_spi('a');
	test_spi('b');
	test_spi('c');
	test_spi('d');
	test_spi('g');

	test_uart('a');
	test_uart('b');
	test_uart('c');
	test_uart('d');
	test_uart('g');
#endif

#if TESTALATOR
	testalator();
#endif

	// Read and run a script.
	main_body();

	while (1);
	return 0;
}

// TODO this should read "debug" in fact
#if 1
/*******************************************************************************
* @brief    Reports the name of the source file and the source line number
*         where the CHECK_PARAM error has occurred.
* @param[in]  file Pointer to the source file name
* @param[in]    line assert_param error line source number
* @return   None
*******************************************************************************/
void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	(void) file;
	(void) line;

	/* Infinite loop */
	while(1);
}
#endif
