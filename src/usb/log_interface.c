#include "usb/tessel_usb.h"
#include "LPC18xx.h"

#define debug_buf_size 1024
static char debug_buf[2][debug_buf_size];
static volatile unsigned debug_pos = 0;
static volatile unsigned debug_bank = 0;
static volatile bool debug_connected = false;

bool usb_debug_set_interface(uint16_t setting) {
	if (setting == 0) {
		debug_connected = false;
		return true;
	} else if (setting == 1) {
		usb_enable_ep(debug_in_ep, USB_EP_TYPE_BULK, 512);
		debug_connected = true;
		return true;
	}
	return false;
}

void debug_try_to_send() {
	if (usb_ep_ready(debug_in_ep) && debug_pos > 0) {
		usb_ep_start_in(debug_in_ep, (uint8_t*) debug_buf[debug_bank], debug_pos, false);
		debug_pos = 0;
		debug_bank ^= 1;
	}
}

void usb_log_write(char level, const char* s, int len) {
	if (!debug_connected) {
		return;
	}

	if (len + 2 > debug_buf_size) len = debug_buf_size - 2;

	if ((debug_pos + len + 2) > debug_buf_size) {
		while ((debug_pos + len) >= debug_buf_size) { }
	}

	__disable_irq();
	debug_buf[debug_bank][debug_pos++] = 1;
	debug_buf[debug_bank][debug_pos++] = level;
	for (int i=0; i<len; i++) {
		debug_buf[debug_bank][debug_pos++] = s[i];
	}
	__enable_irq();

	debug_try_to_send();
}

void tm_log(char level, const char* s, int len) {
	usb_log_write(level, s, len);
}
