// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "hw.h"
#include "variant.h"


/**
 * initialize GPIOs for new script
 */

int tessel_board_version ()
{
  return (*((uint32_t *) 0x4004503C) >> 28);
}


char* tessel_board_uuid ()
{
	static char idbuf[36];
	if (idbuf[0] == 0) {
		unsigned ver = tessel_board_version();
		unsigned id1 = *((uint32_t *) 0x40045000);
		unsigned id2 = *((uint32_t *) 0x40045004);
		unsigned id3 = *((uint32_t *) 0x40045008);
		sprintf(idbuf, "TM-00-%02d-%08x-%08x-%08x", ver, id1, id2, id3);
	}
	return idbuf;
}

#define xstr(s) str(s)
#define str(s) #s

const char* version_info = "{" \
	"\"firmware_git\":" xstr(__TESSEL_FIRMWARE_VERSION__) "," \
	"\"runtime_git\":" xstr(__TESSEL_RUNTIME_VERSION__) "," \
	"\"runtime_version\": \"1.0.0\", " \
	"\"date\":" xstr(__DATE__) "," \
	"\"time\":" xstr(__TIME__) \
	"}";

void tessel_gpio_init (int cc3k_leds)
{
	// LEDs
	hw_digital_output(LED1);
	hw_digital_output(LED2);
	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 0);
	if (cc3k_leds) {
		hw_digital_output(CC3K_CONN_LED);
		hw_digital_output(CC3K_ERR_LED);
		hw_digital_write(CC3K_CONN_LED, 0);
		hw_digital_write(CC3K_ERR_LED, 0);
	}

	hw_digital_startup(A_G1);
	hw_digital_startup(A_G2);
	hw_digital_startup(A_G3);
	
	hw_digital_startup(B_G1);
	hw_digital_startup(B_G2);
	hw_digital_startup(B_G3);
	
	hw_digital_startup(C_G1);
	hw_digital_startup(C_G2);
	hw_digital_startup(C_G3);

	hw_digital_startup(D_G1);
	hw_digital_startup(D_G2);
	hw_digital_startup(D_G3);

	hw_digital_startup(E_G1);
	hw_digital_startup(E_G2);
	hw_digital_startup(E_G3);
	hw_digital_startup(E_G4);
	hw_digital_startup(E_G5);
	hw_digital_startup(E_G6);
}

void tessel_network_reset ()
{
	// This is how TI recommends to close all sockets.
	// http://e2e.ti.com/support/low_power_rf/f/851/t/190380.aspx
	// if (hw_net_is_connected()) {
	// 	closesocket(0);
	// 	closesocket(1);
	// 	closesocket(2);
	// 	closesocket(3);
	// }
}
