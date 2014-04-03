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
