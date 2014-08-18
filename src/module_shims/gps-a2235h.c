// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
#include "gps-a2235h.h"

int gps_init(void) {
	// inits gps with sw uart
	// this is the command buffer to switch the a2235h from 115200 baud and binary to 9600 and nmea
	unsigned char gps_init_buff[32] = {0xA0, 0xA2, 0x00, 0x18, 0x81, 0x02, 0x01, 0x01,
		0x00, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x25, 0x80, 0x01, 0x3A, 0xB0, 0xB3};

	return hw_swuart_transmit(gps_init_buff, sizeof(gps_init_buff), TM_SW_UART_115200);
}
