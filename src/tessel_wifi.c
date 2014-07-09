// Copyright 2014 Technical Machine. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

// Tessel-specific wifi setup

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <lpc_types.h>
#include <lpc18xx_gpio.h>

#include "tessel_wifi.h"
#include "hw.h"
#include "variant.h"
#include "tm.h"
#include "utility/wlan.h"
#include "colony.h"

static uint8_t MAX_CC_BOOT_TICKS = 120;
int wifi_initialized = 0;

volatile int validirqcount = 0;

volatile uint8_t CC3K_EVENT_ENABLED = 0;
volatile uint8_t CC3K_IRQ_FLAG = 0;
int CC3K_callback_err = 0;
char *CC3K_callback_payload = 0; 

struct wifi_status_t {
  int callback_err;
  char* callback_payload;
  bool callback_payload_free;
  bool post_connect;
};

void clear_wifi_events();
void wifi_no_func();
void wifi_connection_callback();

/// The event triggered by the cc callback
tm_event wifi_connect_event = TM_EVENT_INIT(wifi_connection_callback);

struct wifi_status_t wifi_status = {
  .callback_err = 0,
  .callback_payload = 0,
  .callback_payload_free = false,
  .post_connect = false,
};

void wifi_connection_callback (void) {
	// Make sure the Lua state exists
	lua_State* L = tm_lua_state;
	if (!L) return;

	// Push the _colony_emit helper function onto the stack
	lua_getglobal(L, "_colony_emit");
	// The process message identifier
	lua_pushstring(L, "wifi_connect_complete");
	TM_DEBUG("wifi connection emitted %s", wifi_status.callback_payload);
	// push whether we got an error (1 or 0)
	lua_pushnumber(L, wifi_status.callback_err);
	lua_pushstring(L, wifi_status.callback_payload);

	if (wifi_status.callback_payload_free) {
		free(wifi_status.callback_payload);
		wifi_status.callback_payload_free = false;
	}
	
	tm_event_unref(&wifi_connect_event);

	// Call _colony_emit to run the JS callback
	tm_checked_call(L, 3);
}

uint8_t get_cc3k_irq_flag () {
	return CC3K_IRQ_FLAG;
}

void set_cc3k_irq_flag (uint8_t value) {
	CC3K_IRQ_FLAG = value;
}

void SPI_IRQ_CALLBACK_EVENT (tm_event* event)
{
	(void) event;
	CC3K_EVENT_ENABLED = 0;
	if (CC3K_IRQ_FLAG) {
		CC3K_IRQ_FLAG = 0;
		SPI_IRQ();
	}
}

tm_event cc3k_irq_event = TM_EVENT_INIT(SPI_IRQ_CALLBACK_EVENT);

void _tessel_cc3000_irq_interrupt ()
{
	validirqcount++;
	if (GPIO_GetIntStatus(CC3K_GPIO_INTERRUPT))
	{
		CC3K_IRQ_FLAG = 1;
    	if (!CC3K_EVENT_ENABLED) {
    		CC3K_EVENT_ENABLED = 1;
			tm_event_trigger(&cc3k_irq_event);
		}
		GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, CC3K_GPIO_INTERRUPT);
	}
}

void tessel_wifi_init ()
{
	// Turn SW low
	hw_digital_output(CC3K_SW_EN);
	hw_digital_write(CC3K_SW_EN, 0);

	// enable GPIO interrupt for CC3K_IRQ on interrupt #7
	hw_interrupt_enable(CC3K_GPIO_INTERRUPT, CC3K_IRQ, TM_INTERRUPT_MODE_FALLING);
}


#ifdef TESSEL_FASTCONNECT
int cc_blink = 1;
#else
int cc_blink = 0;
#endif
int cc_bootup = 0;
void _cc3000_cb_animation_tick (size_t frame)
{
	if (cc_blink) {
		hw_digital_write(CC3K_CONN_LED, frame & 1 ? 1 : 0);

		if (cc_bootup != -1 && cc_bootup < MAX_CC_BOOT_TICKS) {
			cc_bootup++;
			if (cc_bootup >= MAX_CC_BOOT_TICKS) {
				hw_digital_write(CC3K_CONN_LED, 0);
				cc_blink = 0;
				cc_bootup = -1;
			}
		}
	}
}

void _cc3000_cb_acquire ()
{
	TM_COMMAND('W', "{\"event\": \"acquire\"}");
	TM_DEBUG("wifi acquiring");
	cc_blink = 1;
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 0);
}

void _cc3000_cb_error (int connected)
{
	TM_COMMAND('W', "{\"event\": \"error\", \"error\": %d, \"when\": \"acquire\"}", connected);
	cc_blink = 0;
	hw_digital_write(CC3K_ERR_LED, 1);
	hw_digital_write(CC3K_CONN_LED, 0);
}

void _cc3000_cb_wifi_connect ()
{
	TM_COMMAND('W', "{\"event\": \"connect\"}");
	TM_DEBUG("wifi connect");
	// only dhcp events that come after this are valid
	wifi_status.post_connect = true;
	cc_blink = 1;
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 0);
}

void _cc3000_cb_wifi_disconnect ()
{
	TM_COMMAND('W', "{\"event\": \"disconnect\"}");
	cc_blink = 0;
	hw_digital_write(CC3K_ERR_LED, 1);
	hw_digital_write(CC3K_CONN_LED, 0);
}

void _cc3000_cb_dhcp_failed ()
{
	TM_DEBUG("DHCP failed. Try reconnecting.");
	TM_COMMAND('W', "{\"event\": \"dhcp-failed\"}");
	cc_blink = 0;
	tessel_wifi_check(1);
	hw_digital_write(CC3K_ERR_LED, 1);
	hw_digital_write(CC3K_CONN_LED, 0);
}

void _cc3000_cb_dhcp_success ()
{
	hw_digital_write(CC3K_CONN_LED, 1);
	TM_COMMAND('W', "{\"event\": \"dhcp-success\"}");
	tessel_wifi_check(1);
	cc_blink = 0;
	cc_bootup = -1;
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 1);
}

void _cc3000_cb_tcp_close (int socket)
{
	uint32_t s = socket;
	if (tm_lua_state != NULL) {
		colony_ipc_emit(tm_lua_state, "tcp-close", &s, sizeof(uint32_t));
	}
}


void tessel_wifi_disable ()
{
	wlan_ioctl_set_connection_policy(0, 0, 1);
	hw_net_disconnect();
	// hw_wait_ms(1000);
	wifi_initialized = 0;
} 


void tessel_wifi_enable ()
{
	if (!wifi_initialized) {
		hw_net_initialize();
		wifi_initialized = 1;
		tm_net_initialize_dhcp_server();
	} else {
		// disable and re-enable
		tessel_wifi_disable();
		tessel_wifi_enable();
	}
}


void tessel_wifi_smart_config ()
{
	if (hw_net_online_status()) {
		tessel_wifi_disable();
	}
//  tm_task_idle_start(tm_task_default_loop(), StartSmartConfig, 0);
//  tm_net_initialize();
	StartSmartConfig();
}

#define TM_BYTE(A, B) ((A >> (B*8)) & 0xFF)

void tessel_wifi_check (uint8_t asevent)
{
	(void) asevent;

	if (hw_net_online_status()){
		uint32_t ip_addr = (hw_wifi_ip[3] << 24) | (hw_wifi_ip[2] << 16) | (hw_wifi_ip[1] << 8) | (hw_wifi_ip[0]);
		TM_DEBUG("IP Address: %ld.%ld.%ld.%ld", TM_BYTE(ip_addr, 3), TM_BYTE(ip_addr, 2), TM_BYTE(ip_addr, 1), TM_BYTE(ip_addr, 0));
		uint32_t ip_dns = hw_net_dnsserver();
		TM_DEBUG("DNS: %ld.%ld.%ld.%ld", TM_BYTE(ip_dns, 3), TM_BYTE(ip_dns, 2), TM_BYTE(ip_dns, 1), TM_BYTE(ip_dns, 0));
		uint32_t ip_dhcp = hw_net_dhcpserver();
		TM_DEBUG("DHCP server: %ld.%ld.%ld.%ld", TM_BYTE(ip_dhcp, 3), TM_BYTE(ip_dhcp, 2), TM_BYTE(ip_dhcp, 1), TM_BYTE(ip_dhcp, 0));
		uint32_t ip_gw = hw_net_defaultgateway();
		TM_DEBUG("Default Gateway: %ld.%ld.%ld.%ld", TM_BYTE(ip_gw, 3), TM_BYTE(ip_gw, 2), TM_BYTE(ip_gw, 1), TM_BYTE(ip_gw, 0));
		TM_DEBUG("Connected to WiFi!");

		char ssid[33] = { 0 };
		hw_net_ssid((char*) ssid);

		char* payload = 0;
		asprintf(&payload, "{\"event\": \"status\""
			"," "\"connected\": 1"
			"," "\"ip\": \"%ld.%ld.%ld.%ld\""
			"," "\"dns\": \"%ld.%ld.%ld.%ld\""
			"," "\"dhcp\": \"%ld.%ld.%ld.%ld\"" 
			"," "\"gateway\": \"%ld.%ld.%ld.%ld\""
			"," "\"ssid\": \"%s\""
			"}",
			TM_BYTE(ip_addr, 3), TM_BYTE(ip_addr, 2), TM_BYTE(ip_addr, 1), TM_BYTE(ip_addr, 0),
			TM_BYTE(ip_dns, 3), TM_BYTE(ip_dns, 2), TM_BYTE(ip_dns, 1), TM_BYTE(ip_dns, 0),
			TM_BYTE(ip_dhcp, 3), TM_BYTE(ip_dhcp, 2), TM_BYTE(ip_dhcp, 1), TM_BYTE(ip_dhcp, 0),
			TM_BYTE(ip_gw, 3), TM_BYTE(ip_gw, 2), TM_BYTE(ip_gw, 1), TM_BYTE(ip_gw, 0),
			ssid
		);
		TM_COMMAND('W', payload);

		if (wifi_status.post_connect) {
			wifi_status.callback_err = 0;
			wifi_status.callback_payload = payload;
			wifi_status.callback_payload_free = true;
			wifi_status.post_connect = false;
			// callback
			tm_event_trigger(&wifi_connect_event);
		} else {
			free(payload); 
		}

	} else {
		TM_COMMAND('W', "{\"event\": \"status\""
			"," "\"connected\": 0"
			"}");
		
		if (wifi_status.post_connect) {
			wifi_status.callback_err = 1;
			wifi_status.callback_payload = "Could not get connection";
			wifi_status.callback_payload_free = false;
			wifi_status.post_connect = false;

			tm_event_trigger(&wifi_connect_event);

		}
		
		
	}
}

int tessel_wifi_connect(char * wifi_security, char * wifi_ssid, char* wifi_pass, size_t ssidlen, size_t passlen)
{
	// Check arguments.
	if (wifi_ssid[0] == 0 ) {
		TM_DEBUG("hw_net is in use");
		return 1;
	}

	// if (hw_net_online_status()){
	// 	TM_DEBUG("Disconnecting from current network.");
	// 	TM_COMMAND('W', "{\"connected\": 0}");
	// 	tessel_wifi_disable();
	// }

	TM_DEBUG("Starting up wifi (already initialized: %d)", wifi_initialized);
	// if (!wifi_initialized) {
	// 	tessel_wifi_enable();
	// }

	// Connect to given network.
	hw_net_connect(wifi_security, wifi_ssid, wifi_pass, ssidlen, passlen); // use this for using tessel wifi from command line

	return 0;
}

// tries to connect straight away
void tessel_wifi_fastconnect() {

	TM_DEBUG("Connecting to last available network...");

	hw_net_initialize();
	wifi_initialized = 1;
}
