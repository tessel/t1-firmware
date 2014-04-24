// Copyright 2013 Technical Machine. See the COPYRIGHT
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

#include "hw.h"
#include "variant.h"
#include "tm.h"
#include "utility/wlan.h"

unsigned long wifi_intervals[16] = { 2000 };

int wifi_initialized = 0;


void tessel_wifi_disable ()
{
	hw_net_disconnect();
	hw_wait_ms(100);
	wifi_initialized = 0;
} 


void tessel_wifi_enable ()
{
	if (!wifi_initialized) {
		hw_net_initialize();
		hw_net_config(0, TESSEL_FASTCONNECT, !TESSEL_FASTCONNECT);
		wlan_ioctl_set_scan_params(10000, 20, 30, 2, 0x7FF, -100, 0, 205, wifi_intervals);
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


void tessel_wifi_check(uint8_t output){
	if (hw_net_online_status()){
		TM_DEBUG("IP Address: %ld.%ld.%ld.%ld", hw_wifi_ip[3], hw_wifi_ip[2], hw_wifi_ip[1], hw_wifi_ip[0]);
		uint32_t ip = hw_net_dnsserver();
		TM_DEBUG("DNS: %ld.%ld.%ld.%ld", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
		ip = hw_net_dhcpserver();
		TM_DEBUG("DHCP server: %ld.%ld.%ld.%ld", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
		ip = hw_net_defaultgateway();
		TM_DEBUG("Default Gateway: %ld.%ld.%ld.%ld", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF);
		TM_DEBUG("Connected to WiFi!");
		TM_COMMAND('W', "{\"connected\": 1, \"ip\": \"%ld.%ld.%ld.%ld\"}", hw_wifi_ip[0], hw_wifi_ip[1], hw_wifi_ip[2], hw_wifi_ip[3]);
	} else {
		if (output) TM_COMMAND('W', "{\"connected\": 0, \"ip\": null}");
	}
}

int tessel_wifi_connect(char * wifi_security, char * wifi_ssid, char* wifi_pass)
{
	if (wifi_ssid[0] == 0) {
		return 1;
	}

	if (hw_net_online_status()){
		TM_DEBUG("Disconnecting from current network.");
		TM_COMMAND('W', "{\"connected\": 0}");
		tessel_wifi_disable();
	}

	// TM_DEBUG("Starting up wifi (already initialized: %d)", wifi_initialized);
	if (!wifi_initialized) {
		tessel_wifi_enable();
	}

	// Connect to given network.
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_net_connect(wifi_security, wifi_ssid, wifi_pass); // use this for using tessel wifi from command line

	return 0;
}

// tries to connect straight away
void tessel_wifi_fastconnect() {

	TM_DEBUG("Connecting to last available network...");

	hw_net_initialize();
	hw_digital_write(CC3K_ERR_LED, 0);
	wifi_initialized = 1;
}
