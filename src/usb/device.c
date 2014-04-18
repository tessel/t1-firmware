#include "usb/tessel_usb.h"
#include "tessel.h"
#include "hw.h"

void handle_device_control_setup() {
	switch (usb_setup.bRequest) {
		case REQ_INFO:
			if ((usb_setup.bmRequestType & USB_IN) == USB_IN) {
				unsigned size = strlen(version_info);
				if (size > usb_setup.wLength) size = usb_setup.wLength;
				usb_ep0_out();
				usb_ep_start_in(0x80, (const uint8_t*) version_info, size, true);
				return;
			} 
			break;
		case REQ_KILL:
			main_body_interrupt();
			usb_ep0_out();
			return usb_ep0_in(0);
		case REQ_STACK_TRACE:
			enqueue_system_event(debugstack, 0);
			usb_ep0_out();
			return usb_ep0_in(0);
		case REQ_WIFI:
			if ((usb_setup.bmRequestType & USB_IN) == USB_IN) {
				usb_ep0_out();

				memcpy(ep0_buf_in, hw_wifi_ip, 4);
				return usb_ep0_in(4);
			}
		case REQ_CC:
			if ((usb_setup.bmRequestType & USB_IN) == USB_IN) {
				usb_ep0_out();

				memcpy(ep0_buf_in, hw_cc_ver, 2);
				return usb_ep0_in(2);
			}
			break;
	}
	return usb_ep0_stall();

}
