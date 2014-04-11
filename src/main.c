#define TESSEL_WIFI 1
#define TESSEL_WIFI_DEPLOY 0
#define TESSEL_TEST 0
#define TESSEL_FASTCONNECT 1
#define TESTALATOR 0

#ifndef COLONY_STATE_CACHE
#define COLONY_STATE_CACHE 0
#endif
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

#include "linker.h"

#include "LPC18xx.h"                        /* LPC18xx definitions */
#include "lpc18xx_libcfg.h"
#include "lpc_types.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_gpio.h"
#include "lpc18xx_gpdma.h"

#include "spifi_rom_api.h"

#include "tm.h"
#include "hw.h"
#include "tessel.h"
#include "l_hw.h"
#include "colony.h"
#include "test.h"

#include "sdram_init.h"
#include "lpc_types.h"
#include "spi_flash.h"
#include "bootloader.h"
#include "tm_event_queue.h"
#include "utility/wlan.h"

// lua
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

tm_fs_ent* tm_fs_root;

/**
 * Custom awful CC3000 code
 */

volatile int validirqcount = 0;
volatile int netconnected = 0;

volatile uint8_t CC3K_EVENT_ENABLED = 0;
volatile uint8_t CC3K_IRQ_FLAG = 0;

uint8_t get_cc3k_irq_flag () {
	// volatile uint8_t read = !hw_digital_read(CC3K_IRQ);
	// (void) read;
	return CC3K_IRQ_FLAG;
}

void set_cc3k_irq_flag (uint8_t value) {
	CC3K_IRQ_FLAG = value;
	// hw_digital_write(CC3K_ERR_LED, value);
}

void SPI_IRQ_CALLBACK_EVENT (unsigned no)
{
	(void) no;
	CC3K_EVENT_ENABLED = 0;
	// hw_digital_write(CC3K_ERR_LED, 0);
	if (CC3K_IRQ_FLAG) {
		CC3K_IRQ_FLAG = 0;
		SPI_IRQ();
	}
}

void __attribute__ ((interrupt)) GPIO0_IRQHandler(void)
{
	validirqcount++;
	if (GPIO_GetIntStatus(0))
	{
		// hw_digital_write(CC3K_ERR_LED, 1);
		CC3K_IRQ_FLAG = 1;
    	if (!CC3K_EVENT_ENABLED) {
    		CC3K_EVENT_ENABLED = 1;
			enqueue_system_event(SPI_IRQ_CALLBACK_EVENT, 0);
		}
		GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, 0);
	}
}


void __attribute__ ((interrupt)) GPIO1_IRQHandler(void)
{
	SPI_IRQ();
}

void smart_config_task ();

/**
 * Smartconfig Button
 */

void __attribute__ ((interrupt)) GPIO3_IRQHandler(void)
{
  if (GPIO_GetIntStatus(3))
  {
    if (smartconfig_process == 0) {
      smartconfig_process = 1;
      //tm_task_interruptall(tm_task_default_loop(), smart_config_task);
      smart_config_task();
    }
    GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, 3);
    GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, 3);
  }
}

void TM_NET_INT_INIT ()
{
	// Turn SW low
	hw_digital_output(CC3K_SW_EN);
	hw_digital_write(CC3K_SW_EN, 0);

	// Add GPIO Interrupt #2
//	 attachGPIOInterrupt0(0, 7, 0x2, 7, MODE_FALLING);
//	attachGPIOInterrupt0(0, 12, 0x1, 17, MODE_FALLING); // P3.1 : GPIO5_8,
	hw_interrupt_listen(0, CC3K_IRQ, TM_INTERRUPT_MODE_FALLING);
//	attachGPIOInterrupt0(2, 2, 0x4, 2, MODE_FALLING);

	// Add GPIO Interrupt #1 as a horrible workaround for NVIC
	NVIC_SetPriority(PIN_INT1_IRQn, ((0x03<<3)|0x02));
	/* Enable interrupt for Pin Interrupt */
	NVIC_EnableIRQ(PIN_INT1_IRQn);
}

void TM_SMARTCONFIG_INT_INIT() {
  hw_interrupt_listen(3, CC3K_CONFIG, TM_INTERRUPT_MODE_FALLING);

  // Add GPIO Interrupt #2 as a horrible workaround for NVIC
  NVIC_SetPriority(PIN_INT3_IRQn, ((0x03<<3)|0x02)); // TODO: is this the wrong priority?
  /* Enable interrupt for Pin Interrupt */
  NVIC_EnableIRQ(PIN_INT3_IRQn);
}

/*----------------------------------------------------------------------------
  DMA
 *---------------------------------------------------------------------------*/

void DMA_IRQHandler (void)
{
	// check GPDMA interrupt on channel 0
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)){ 
		hw_spi_dma_counter(0);
	}

	// check GPDMA interrupt on channel 1
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, 1)){ 
		hw_spi_dma_counter(1);
	}

}


/*----------------------------------------------------------------------------
  Main Program
 *---------------------------------------------------------------------------*/

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
 * wifi connect
 */

// remote wifi deploy
volatile int tcpsocket = -1;
volatile int tcpclient = -1;
volatile int accept_count = 0;

char wifi_ssid[32] = {0};
char wifi_pass[64] = {0};
char wifi_security[32] = {0};
static int wifi_timeout = 8000;

unsigned long wifi_intervals[16] = { 2000 };

int wifi_initialized = 0;

void tessel_wifi_enable ()
{
	if (!wifi_initialized) {
		hw_net_initialize();
		hw_net_config(0, TESSEL_FASTCONNECT, !TESSEL_FASTCONNECT);
		wlan_ioctl_set_scan_params(10000, 20, 30, 2, 0x7FF, -100, 0, 205, wifi_intervals);
		wifi_initialized = 1;
		tm_net_initialize_dhcp_server();
	}
}

void tessel_wifi_disable ()
{
	hw_net_disconnect();
	hw_wait_ms(10);
	netconnected = 0;
	wifi_initialized = 0;
} 

void smart_config_task ()
{
	if (netconnected != 0) {
		tessel_wifi_disable();
	}
//  tm_task_idle_start(tm_task_default_loop(), StartSmartConfig, 0);
//  tm_net_initialize();
	StartSmartConfig();
}

void on_wifi_connected()
{
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 1);

	uint32_t ip = 0;
	hw_net_populate_ip();
	TM_DEBUG("IP Address: %ld.%ld.%ld.%ld", hw_wifi_ip[0], hw_wifi_ip[1], hw_wifi_ip[2], hw_wifi_ip[3]);
	ip = hw_net_dnsserver();
	TM_DEBUG("DNS: %ld.%ld.%ld.%ld", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
	ip = hw_net_dhcpserver();
	TM_DEBUG("DHCP server: %ld.%ld.%ld.%ld", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
	ip = hw_net_defaultgateway();
	TM_DEBUG("Default Gateway: %ld.%ld.%ld.%ld", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
	TM_DEBUG("Connected to WiFi!");

	TM_COMMAND('W', "{\"connected\": 1, \"ip\": \"%ld.%ld.%ld.%ld\"}", hw_wifi_ip[0], hw_wifi_ip[1], hw_wifi_ip[2], hw_wifi_ip[3]);

#if TESSEL_WIFI_DEPLOY
	// Enable remote deploy on 4444
	tcpsocket = tm_tcp_open();
	tm_tcp_listen(tcpsocket, 4444);
	TM_COMMAND('u', "Listening for remote updates on port 4444 (socket %d)...", tcpsocket);
#endif

	netconnected = 1;
}

int connect_wifi ()
{
	if (wifi_ssid[0] == 0) {
		return 1;
	}

	if (netconnected == 1){
		TM_DEBUG("Disconnecting from current network.");
		TM_COMMAND('W', "{\"connected\": 0}");
		tessel_wifi_disable();
		
	}

	TM_DEBUG("Starting up wifi (already initialized: %d)", wifi_initialized);
	tessel_wifi_enable();

	// Connect to given network.
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_net_connect(wifi_security, wifi_ssid, wifi_pass); // use this for using tessel wifi from command line

	TM_DEBUG("Waiting for DHCP (%d second timeout)...", wifi_timeout / 1000);
	// DHCP wait can return but if local IP is 0, consider it a timeout.
	hw_net_populate_ip();
	if (!hw_net_block_until_dhcp_wait(wifi_timeout) || 
		(hw_wifi_ip[0] | hw_wifi_ip[1] | hw_wifi_ip[2] | hw_wifi_ip[3]) == 0) {
		wifi_ssid[0] = 0;
		TM_DEBUG("Connection timed out. Try connecting to WiFi again.");
		TM_COMMAND('W', "{\"connected\": 0, \"ip\": null}");
		return 1;
	}

	on_wifi_connected();
	return 0;
}

// tries to connect straight away
int fast_connect() {

	TM_DEBUG("Connecting to last available network...");
	int backoff = 2000;

	hw_net_initialize();
	hw_digital_write(CC3K_ERR_LED, 0);
	if (hw_net_block_until_dhcp_wait(backoff)) {
		TM_DEBUG("Connected to saved network.");
		netconnected = 1;
		on_wifi_connected();
		return 1;
	}

	tessel_wifi_disable();
	TM_DEBUG("Couldn't connect to saved network.");
	TM_COMMAND('W', "{\"connected\": 0, \"ip\": null}");

	tessel_wifi_enable();
	return 0;
}


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
		}
		else if (cmd == 'W') {
#if !TESSEL_WIFI
			TM_ERR('w', "WiFi command not enabled on this Tessel.\n");
#else
			if (size < 32 + 64 + 32) {
				TM_ERR("WiFi buffer malformed, aborting.");
				TM_COMMAND('W', "{\"connected\":0}");
			} else {
				memcpy(wifi_ssid, &buf[0], 32);
				memcpy(wifi_pass, &buf[32], 64);
				memcpy(wifi_security, &buf[96], 32);
				if (size == 32 + 64 + 32 + 1) {
					wifi_timeout = 1000 * (uint8_t) buf[128];
				}
				main_body_interrupt();
				script_buf_lock = SCRIPT_EMPTY;
			}
#endif
		}
		else if (cmd == 'V') {
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

				char* nameptr = (char*) &results[0 + 4 + 4 + 1 + 1 + 2];
				unsigned char namebuf[33] = { 0 };
				memcpy(namebuf, nameptr, 32);
				tm_logf('V', "\t%s", namebuf);
			}

			if (hw_net_is_connected()) {
				TM_COMMAND('V', "{\"connected\": 1, \"ip\": \"%ld.%ld.%ld.%ld\"}", hw_wifi_ip[0], hw_wifi_ip[1], hw_wifi_ip[2], hw_wifi_ip[3]);
			} else {
				TM_COMMAND('V', "{\"connected\": 0}");
			}
		}
		else if (cmd == 'M') {
			script_msg_queue("message", buf, size);
		}
		else if (cmd == 'B') {
			jump_to_flash(FLASH_BOOT_ADDR, BOOT_MAGIC);
			while(1);
		}
		else {
			// Invalid?
			script_buf_lock = SCRIPT_EMPTY;
		}

		free(buf);
}

/**
 * remote wifi push
 */

tessel_cmd_buf_t cmd_wifi;

void wifipoll_close ()
{
	tm_tcp_close(tcpclient);
	tcpclient = -1;
}

size_t wifipoll_read (uint8_t *buf, size_t len)
{
	if (tcpclient < 0) {
		return 0;
	}

	ssize_t read_len = tm_tcp_read(tcpclient, buf, len);
	if (read_len < 0) {
		// Negative length probably means closed client.
		wifipoll_close();
		return 0;
	}
	return (size_t) read_len;
}

void systick_wifipoll ()
{
#if TESSEL_WIFI_DEPLOY
	uint32_t ip;
	if (tcpclient < 0) {
		tcpclient = tm_tcp_accept(tcpsocket, &ip);
//		TM_DEBUG("client %d", tcpclient);

		// One major pause just in case.
		if (tcpclient >= 0) {
			hw_wait_ms(100);
			TM_DEBUG("");
			TM_DEBUG("[wifi command]");
			output_wifi = 1;
			output_unbuffer();
		}
	}

	while (tcpclient >= 0 && !hw_net_inuse()) {
//		TM_DEBUG("--> inuse %d", hw_net_inuse());
		if (!tm_tcp_readable(tcpclient)) {
			break;
		}
		tessel_cmd_process(&cmd_wifi, wifipoll_read);
	}
#endif
}


/**
 * system tick handler
 */

// SysTick Interrupt Handler @ 1000Hz
_ramfunc void SysTick_Handler (void)
{
#if TESSEL_WIFI_DEPLOY
	if (!netconnected || accept_count-- > 0 || hw_net_inuse()) {
		return;
	}
	accept_count = 1000;

	systick_wifipoll();
#endif
}


/**
 * Main body of Tessel OS
 */

jmp_buf L_jmp;
int luaExitCode = 0;

bool lua_exiting = 0;
volatile event_ringbuf system_event_queue = {0, 0, {{0}}};

/// Atomically sets a lua hook by disabling interrupts
void safe_sethook(lua_Hook func, int mask, int count) {
	unsigned primask = __get_PRIMASK(); // Interrupt status
	__disable_irq();
	if (tm_lua_state != NULL) {
		lua_sethook(tm_lua_state, func, mask, count);
	}
	__set_PRIMASK(primask); // Restore interrupt status
}

void queue_hook (lua_State* L, lua_Debug *ar)
{
	(void) ar;
	safe_sethook(NULL, 0, 0);
	while (pop_event(&system_event_queue)) {
		if (lua_exiting) {
			 lua_exiting = false;
			 if (L != NULL) {
				longjmp(L_jmp, 1);
			}
		}
	}
}

void enqueue_system_event(event_callback callback, unsigned data) {
	enqueue_event(&system_event_queue, callback, data);
	safe_sethook(queue_hook, LUA_MASKCOUNT, 1);
}

void main_body_interrupt_hook (unsigned data)
{
	(void) data;
	lua_exiting = true;
}

void main_body_interrupt ()
{
	if (tm_lua_state != NULL) {
		enqueue_system_event(main_body_interrupt_hook, 0);
	}
}

static int main_body_os_exit (lua_State *L)
{
	luaExitCode = luaL_optint(L, 1, EXIT_SUCCESS);
	main_body_interrupt();
	return 0;
}

void script_msg_queue (char *type, void* data, size_t size) {
	lua_State* L = tm_lua_state;
	// Get preload table.
	lua_getglobal(L, "_colony_ipc");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
	} else {
		size_t idx = lua_objlen(L, -1) + 1;

		lua_createtable(L, 2, 2);
		lua_pushstring(L, type);
		lua_rawseti(L, -2, 1);
		lua_pushlstring(L, data, size);
		lua_rawseti(L, -2, 2);

		lua_rawseti(L, -2, idx);
		lua_pop(L, 1);
	}
}

void debugstack(unsigned _)
{
	(void) _;
	lua_State* L = tm_lua_state;
	if (tm_lua_state == 0) {
		hw_send_usb_msg('k', NULL, 0);
		return;
	}

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
//        // Dont' reconnect.
//        if (netconnected != 1) {
//                tm_net_disconnect();
//                netconnected = 0;
//        }

		// While we're not running the script, we can do other interrupts.
		TM_DEBUG("Ready.");
		while (script_buf_lock != SCRIPT_READING) {
			if (wifi_ssid[0] != 0) {
				connect_wifi();
				wifi_ssid[0] = 0;
			}
			pop_event(&system_event_queue);
			tm_event_process();
			continue;
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

#if COLONY_STATE_CACHE
#define RUNTIME_ARENA_SIZE 1024*1024*8
void* runtime_cache = NULL;
size_t runtime_cache_len = 0;
void* runtime_arena = NULL;
#endif

void tm_events_lock() { __disable_irq(); }
void tm_events_unlock() { __enable_irq(); }

void load_script(uint8_t* script_buf, unsigned script_buf_size, uint8_t speculative)
{
		int ret = 0;

		initialize_GPIO_interrupts();

		// Reset board state.
		tessel_gpio_init(0);
		tessel_network_reset();

		// Populate filesystem.
		TM_DEBUG("Populating filesystem...");
		tm_fs_root = tm_fs_dir_create();
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

		// arena with clone test
#if COLONY_STATE_CACHE
		if (runtime_cache == NULL) {
			runtime_arena = malloc(RUNTIME_ARENA_SIZE);
#endif

			// Open runtime.
#if COLONY_STATE_CACHE
			TM_DEBUG("Initializing cachable runtime...");
			colony_runtime_arena_open(runtime_arena, RUNTIME_ARENA_SIZE, COLONY_PRELOAD_ON_INIT);
#else
			TM_DEBUG("Initializing runtime...");
			assert(tm_lua_state == NULL);
			colony_runtime_open();
#endif

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

			// Get preload table.
			lua_pushcfunction(L, main_body_os_exit);
			lua_setglobal(L, "_exit");

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

#if COLONY_STATE_CACHE
			// Preserve state
			runtime_cache_len = colony_runtime_arena_save_size(runtime_arena, RUNTIME_ARENA_SIZE);
			runtime_cache = malloc(runtime_cache_len);
			TM_DEBUG("Caching state in %d bytes...\n", runtime_cache_len);
			colony_runtime_arena_save(runtime_arena, RUNTIME_ARENA_SIZE, runtime_cache, runtime_cache_len);
		} else {
			TM_DEBUG("Restoring runtime state...");
			memset(runtime_arena, 0, RUNTIME_ARENA_SIZE);
			colony_runtime_arena_restore(runtime_arena, RUNTIME_ARENA_SIZE, runtime_cache, runtime_cache_len);
		}
#endif

		const char *argv[3];
		argv[0] = "runtime";
		if (start_script_is_indexjs) {
			argv[1] = "index.js";
		} else {
			argv[1] = "_start.js";
		}
		argv[2] = NULL;
		luaExitCode = 0;
		if (setjmp(L_jmp) == 0) {
			// Run the hook after the first instruction so we don't lose any events
			safe_sethook(queue_hook, LUA_MASKCOUNT, 1);

			TM_COMMAND('S', "1");
			TM_LOG("Running script...");
			TM_DEBUG("Uptime since startup: %fs", ((float) tm_uptime_micro()) / 1000000.0);
			ret = colony_runtime_run(argv[1], argv, 2);

			while (tm_events_active()) {
				tm_event_trigger(&tm_timer_event);
				tm_event_process();
				pop_event(&system_event_queue);
			}
		}
#if !COLONY_STATE_CACHE
		colony_runtime_close();
#endif
		tm_fs_destroy(tm_fs_root);
		tm_fs_root = NULL;

		int returncode = ret == 0 ? luaExitCode : ret;
		TM_COMMAND('S', "%d", -returncode);
		TM_DEBUG("Script ended with return code %d.", returncode);
}

#include <internal.h>
#include <sys/stat.h>
#include <fcntl.h>

// TODO get rid of these
void *getgrgid(gid_t gid) { (void) gid; return 0; }
void *getpwuid(pid_t gid) { (void) gid; return 0; }
void *getgrnam(const char *name) { (void) name; return 0; }
void *getpwnam(const char *name) { (void) name; return 0; }
void strmode(int mode, char *bp) { (void) mode; *bp = 0; }

void tm_usb_init(void);

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

	tm_usb_init();

	// Run tests forever.
//	test_neopixel();
//	test_leds();

	// Control these outside of tessel_gpio_init()
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);

	// Interrupt at 1000hz
#if TESSEL_WIFI_DEPLOY
	NVIC_SetPriority(SysTick_IRQn, ((0x02<<3)|0x01));
	SysTick_Config(CGU_GetPCLKFrequency(CGU_PERIPHERAL_M3CORE)/1000);
#endif

#if TESSEL_WIFI
	// CC interrupts
	TM_NET_INT_INIT();
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

#if TESSEL_WIFI && TESSEL_FASTCONNECT
	fast_connect();
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
//	script_buf_size = builtin_tar_gz_len;
//	memcpy(script_buf, &builtin_tar_gz, builtin_tar_gz_len);
//	script_buf_lock = SCRIPT_READING;

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
