// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#pragma once
#include "usb.h"

#define debug_in_ep 0x81
#define msg_in_ep 0x82
#define msg_out_ep 0x02

bool usb_debug_set_interface(uint16_t setting);
void debug_try_to_send(void);

void usb_msg_init(uint16_t altsetting);
void handle_msg_completion(void);

void handle_device_control_setup();

#define REQ_INFO 0x00
#define REQ_KILL 0x10
#define REQ_STACK_TRACE 0x11
#define REQ_WIFI 0x20
#define REQ_CC 0x21
