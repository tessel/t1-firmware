#include "usb.h"
#include "usb/tessel_usb.h"
#include "tessel.h"

void tm_usb_init() {
	usb_init();
	usb_attach();
}

const USB_DeviceDescriptor device_descriptor = {
	.bLength = sizeof(USB_DeviceDescriptor),
	.bDescriptorType = USB_DTYPE_Device,

	.bcdUSB                 = 0x0200,
	.bDeviceClass           = 0,
	.bDeviceSubClass        = 0,
	.bDeviceProtocol        = 0,

	.bMaxPacketSize0        = 64,
	.idVendor               = 0x1d50,
	.idProduct              = 0x6097,
	.bcdDevice              = 0x0101,

	.iManufacturer          = 0x01,
	.iProduct               = 0x02,
	.iSerialNumber          = 0xf0,

	.bNumConfigurations     = 1
};

typedef struct ConfigDesc {
	USB_ConfigurationDescriptor config;
	USB_InterfaceDescriptor intf_off;
	USB_EndpointDescriptor debug_in_off;
	USB_EndpointDescriptor msg_in_off;
	USB_EndpointDescriptor msg_out_off;
	USB_InterfaceDescriptor intf_on;
	USB_EndpointDescriptor debug_in;
	USB_EndpointDescriptor msg_in;
	USB_EndpointDescriptor msg_out;

} ConfigDesc;

const ConfigDesc configuration_descriptor = {
	.config = {
		.bLength = sizeof(USB_ConfigurationDescriptor),
		.bDescriptorType = USB_DTYPE_Configuration,
		.wTotalLength  = sizeof(ConfigDesc),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIG_ATTR_BUSPOWERED,
		.bMaxPower = USB_CONFIG_POWER_MA(500)
	},
	.intf_off = {
		.bLength = sizeof(USB_InterfaceDescriptor),
		.bDescriptorType = USB_DTYPE_Interface,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 3,
		.bInterfaceClass = USB_CSCP_VendorSpecificClass,
		.bInterfaceSubClass = 0xD0,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0x10
	},
	.debug_in_off = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = debug_in_ep,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 512,
		.bInterval = 0x00
	},
	.msg_in_off = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = msg_in_ep,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 512,
		.bInterval = 0x00
	},
	.msg_out_off = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = msg_out_ep,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 512,
		.bInterval = 0x00
	},
	.intf_on = {
		.bLength = sizeof(USB_InterfaceDescriptor),
		.bDescriptorType = USB_DTYPE_Interface,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 1,
		.bNumEndpoints = 3,
		.bInterfaceClass = USB_CSCP_VendorSpecificClass,
		.bInterfaceSubClass = 0xD0,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0x11
	},
	.debug_in = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = debug_in_ep,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 512,
		.bInterval = 0x00
	},
	.msg_in = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = msg_in_ep,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 512,
		.bInterval = 0x00
	},
	.msg_out = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = msg_out_ep,
		.bmAttributes = (USB_EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 512,
		.bInterval = 0x00
	},
};

const USB_StringDescriptor language_string = {
	.bLength = USB_STRING_LEN(1),
	.bDescriptorType = USB_DTYPE_String,
	.bString = {USB_LANGUAGE_EN_US},
};

const USB_StringDescriptor msft_os = {
	.bLength = 18,
	.bDescriptorType = USB_DTYPE_String,
	.bString = {'M','S','F','T','1','0','0',0xee},
};


const USB_MicrosoftCompatibleDescriptor msft_compatible = {
	.dwLength = sizeof(USB_MicrosoftCompatibleDescriptor) + 1*sizeof(USB_MicrosoftCompatibleDescriptor_Interface),
	.bcdVersion = 0x0100,
	.wIndex = 0x0004,
	.bCount = 1,
	.reserved = {0, 0, 0, 0, 0, 0, 0},
	.interfaces = {
		{
			.bFirstInterfaceNumber = 0,
			.reserved1 = 0,
			.compatibleID = "WINUSB\0\0",
			.subCompatibleID = {0, 0, 0, 0, 0, 0, 0, 0},
			.reserved2 = {0, 0, 0, 0, 0, 0},
		},
	}
};

uint16_t usb_cb_get_descriptor(uint8_t type, uint8_t index, const uint8_t** ptr) {
	const void* address = NULL;
	uint16_t size    = 0;

	switch (type) {
		case USB_DTYPE_Device:
			address = &device_descriptor;
			size    = sizeof(USB_DeviceDescriptor);
			break;
		case USB_DTYPE_Configuration:
			address = &configuration_descriptor;
			size    = sizeof(ConfigDesc);
			break;
		case USB_DTYPE_String:
			switch (index) {
				case 0x00:
					address = &language_string;
					break;
				case 0x01:
					address = usb_string_to_descriptor("Technical Machine");
					break;
				case 0x02:
					address = usb_string_to_descriptor("Tessel");
					break;
				case 0xee:
					address = &msft_os;
					break;
				case 0x10:
					address = usb_string_to_descriptor("Tessel Interface");
					break;
				case 0x11:
					address = usb_string_to_descriptor("Tessel Interface (connected)");
					break;
				case 0xf0:
					address = usb_string_to_descriptor(tessel_board_uuid());
					break;
			}
			size = (((USB_StringDescriptor*)address))->bLength;
			break;
	}

	*ptr = address;
	return size;
}

void usb_cb_reset(void) {
}

bool usb_cb_set_configuration(uint8_t config) {
	if (config <= 1) {
		return true;
	} else {
		return false;
	}
}

void usb_cb_completion(void) {
	debug_try_to_send();
	handle_msg_completion();
}

void usb_cb_control_setup(void) {
	uint8_t recipient = usb_setup.bmRequestType & USB_REQTYPE_RECIPIENT_MASK;
	if (recipient == USB_RECIPIENT_DEVICE) {
		if (usb_setup.bRequest == 0xee) {
			return usb_handle_msft_compatible(&msft_compatible);
		}

		if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_VENDOR){
			return handle_device_control_setup();
		}
	}
	return usb_ep0_stall();
}

void usb_cb_control_in_completion(void) {

}

void usb_cb_control_out_completion(void) {

}

bool usb_cb_set_interface(uint16_t interface, uint16_t altsetting) {
	if (interface == 0) {
		usb_msg_init(altsetting);
		return usb_debug_set_interface(altsetting);
	}
	return false;
}

