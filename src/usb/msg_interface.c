#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "tessel.h"
#include "usb/tessel_usb.h"
#include "tm.h"
#include "tessel.h"

void msg_out_rearm_ep(void);
void msg_in_event(unsigned);
void msg_out_event(unsigned);
void msg_cleanup_event(unsigned);
bool usb_msg_connected = 0;

void usb_msg_init(uint16_t altsetting) {
	usb_msg_connected = altsetting;
	if (usb_msg_connected) {
		usb_enable_ep(msg_in_ep, USB_EP_TYPE_BULK, 512);
		usb_enable_ep(msg_out_ep, USB_EP_TYPE_BULK, 512);
		msg_out_rearm_ep();
	} else {
		usb_set_stall_ep(msg_in_ep);
		usb_set_stall_ep(msg_out_ep);
		enqueue_system_event(msg_cleanup_event, usb_msg_connected);
	}
}

inline static usb_size round_to_endpoint_size(unsigned size, unsigned epsize) {
	return ((size + epsize) / epsize) * epsize;
}

static const unsigned msg_epsize = 512;
static const unsigned msg_max_blocksize = 16384;
static const unsigned msg_header_size = 8;


static unsigned msg_out_length = 0;
static unsigned msg_out_pos = 0;
static uint8_t* msg_out_buf = 0;

static uint8_t msg_out_initial[512];

typedef struct message_list_item {
	struct message_list_item* next;
	unsigned length;
	unsigned tag;
	uint8_t data[];
} message_list_item;

message_list_item* in_head = NULL;
message_list_item* in_tail = NULL;
static unsigned msg_in_pos = 0;

void msg_in_start_ep(void) {
	unsigned remaining = in_head->length + msg_header_size - msg_in_pos;
	if (remaining > msg_max_blocksize) {
		usb_ep_start_in(msg_in_ep, ((uint8_t*)in_head) + 4, msg_max_blocksize, false);
		msg_in_pos += msg_max_blocksize;
	} else if (remaining > 0) {
		usb_ep_start_in(msg_in_ep, ((uint8_t*)in_head) + 4 + msg_in_pos, remaining, true);
		msg_in_pos += remaining;
	} else {
		// Notify completion
		enqueue_system_event(msg_in_event, 0);
	}
}

int hw_send_usb_msg(unsigned tag, const uint8_t* msg, unsigned length) {
	if (!usb_msg_connected) {
		return -1;
	}
	message_list_item* item = malloc(sizeof(message_list_item) + length);
	item->next = NULL;
	item->tag = tag;
	item->length = length;
	memcpy(item->data, msg, length);

	if (in_head == NULL) {
		in_head = in_tail = item;
		msg_in_pos = 0;
		msg_in_start_ep();
	} else {
		in_tail->next = item;
		in_tail = item;
	}
	return 0;
}

int hw_send_usb_msg_formatted(unsigned tag, const char* format, ...) {
	va_list args;
	va_start(args, format);
	char buf[256];
	int len = vsnprintf(buf, sizeof(buf), format, args);
	va_end (args);
	return hw_send_usb_msg(tag, (uint8_t*) buf, len);
}

void msg_out_start_ep(void) {
	while (!usb_ep_ready(msg_out_ep)) {}
	unsigned size = msg_max_blocksize;
	unsigned remaining = msg_out_length - msg_out_pos;
	if (remaining < size) {
		size = round_to_endpoint_size(remaining, msg_epsize);
	}
	usb_ep_start_out(msg_out_ep, &msg_out_buf[msg_out_pos], size);
}

void msg_out_rearm_ep(void) {
	usb_ep_start_out(msg_out_ep, msg_out_initial, sizeof(msg_out_initial));
}

void handle_msg_completion() {
	while (usb_ep_pending(msg_in_ep)) {
		usb_ep_handled(msg_in_ep);
		msg_in_start_ep();
	}

	while (usb_ep_pending(msg_out_ep)) {
		unsigned received = usb_ep_out_length(msg_out_ep);
		usb_ep_handled(msg_out_ep);

		if (msg_out_buf == 0) {
			if (received >= msg_header_size) {
				msg_out_pos = received - msg_header_size;
				enqueue_system_event(msg_out_event, 0);
			} else {
				TM_DEBUG("Invalid short packet on msg_out endpoint");
				msg_out_rearm_ep();
			}
		} else {
			msg_out_pos += received;
			if (received < msg_max_blocksize || msg_out_pos > msg_out_length) {
				enqueue_system_event(msg_out_event, 0);
			} else {
				msg_out_start_ep();
			}
		}
	}
}

void msg_cleanup_event(unsigned _) {
	(void) _;
	if (msg_out_buf) {
		free(msg_out_buf);
	}
	msg_out_buf = NULL;
	msg_out_pos = 0;
	msg_out_length = 0;

	while (in_head) {
		message_list_item* item = in_head;
		in_head = item->next;
		free(item);
	}
	msg_in_pos = 0;
}

void msg_out_event(unsigned _) {
	(void) _;
	if (msg_out_buf == 0) {
		memcpy(&msg_out_length, msg_out_initial, 4);
		msg_out_buf = malloc(msg_out_length);
		memcpy(msg_out_buf, msg_out_initial+msg_header_size, msg_out_pos);

		if (msg_out_length + msg_header_size >= sizeof(msg_out_initial)) {
			// Now that the buffer is allocated, receive the rest of the data into it
			return msg_out_start_ep();
		}
	}

	if (msg_out_pos == msg_out_length) {
		unsigned tag;
		memcpy(&tag, msg_out_initial+4, 4);

		if (tag >> 24 == 0) {
			// Pass to original command processor
			tessel_cmd_process(tag & 0xFF, msg_out_buf, msg_out_length);
		} else if (tag >> 24 == 0xAA) {
			// Echo
			hw_send_usb_msg(tag, msg_out_buf, msg_out_length);
			free(msg_out_buf);
		} else {
			TM_DEBUG("Invalid tag");
			free(msg_out_buf);
		}

	} else {
		TM_DEBUG("Invalid message length on msg_out endpoint: %u, expected %u", msg_out_pos, msg_out_length);
	}

	msg_out_buf = NULL;
	msg_out_pos = 0;
	msg_out_length = 0;
	msg_out_rearm_ep();
}

void msg_in_event(unsigned _) {
	(void) _;

	msg_in_pos = 0;
	message_list_item* item = in_head;
	in_head = item->next;

	free(item);

	if (in_head) {
		msg_in_start_ep();	
	}
}
