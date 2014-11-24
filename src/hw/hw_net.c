// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <string.h>
#include <ctype.h>

#include "hw.h"
#include "tessel.h"
#include "tm.h"
#include "host_spi.h"
#include "utility/nvmem.h"
#include "utility/wlan.h"
#include "utility/hci.h"
#include "utility/security.h"
//#include "utility/os.h"
#include "utility/netapp.h"
#include "utility/evnt_handler.h"

/**
 * Configuration
 */

static volatile int inuse = 0;
uint8_t hw_wifi_ip[4] = {0, 0, 0, 0};
uint8_t hw_cc_ver[2] = {0, 0};

void hw_net__inuse_start (void)
{
	inuse = 1;
}

void hw_net__inuse_stop (void)
{
	inuse = 0;
}

#define CC3000_START hw_net__inuse_start();
#define CC3000_END hw_net__inuse_stop();

int hw_net_inuse ()
{
	// If a tm_net device call is being made, indicate so.
	return inuse;
}

int hw_net_erase_profiles()
{
	int deleted = wlan_ioctl_del_profile(255);
	// power cycle
	hw_net_disable();
	hw_wait_ms(10);
	hw_net_initialize();
	ulCC3000Connected = 0;
	ulCC3000DHCP = 0;
	return deleted;
}

int hw_net_is_connected ()
{
	return ulCC3000Connected;
}

int hw_net_has_ip ()
{
	return ulCC3000DHCP;
}

int hw_net_online_status(){
	// checks if the CC is connected to wifi and an IP address is allocated
	return ulCC3000Connected && ulCC3000DHCP;
}

int hw_net_ssid (char ssid[33])
{
	if (hw_net_online_status()) {
		CC3000_START;
		tNetappIpconfigRetArgs ipinfo;
		netapp_ipconfig(&ipinfo);
		memset(ssid, 0, 33);
		memcpy(ssid, ipinfo.uaSSID, 32);
		CC3000_END;
	} else {
		memset(ssid, 0, 33);
	}
	return 0;
}

uint32_t hw_net_defaultgateway ()
{
	CC3000_START;
	tNetappIpconfigRetArgs ipinfo;
	netapp_ipconfig(&ipinfo);
	CC3000_END;

	char* aliasable_ip = (char*) ipinfo.aucDefaultGateway;
	return *((uint32_t *) aliasable_ip);
}

uint32_t hw_net_dhcpserver ()
{
	CC3000_START;
	tNetappIpconfigRetArgs ipinfo;
	netapp_ipconfig(&ipinfo);
	CC3000_END;

	char* aliasable_ip = (char*) ipinfo.aucDHCPServer;
	return *((uint32_t *) aliasable_ip);
}

uint32_t hw_net_dnsserver ()
{
	CC3000_START;
	tNetappIpconfigRetArgs ipinfo;
	netapp_ipconfig(&ipinfo);
	CC3000_END;

	char* aliasable_ip = (char*) ipinfo.aucDNSServer;
	return *((uint32_t *) aliasable_ip);
}

// int tm_net_rssi ()
// {
//   uint8_t results[64];
//   int res = wlan_ioctl_get_scan_results(10000, results);
//   if (res == 0) {
//     return results[4 + 4] & 0x7F; // lower 7 bits
//   }
//   return res;
// }


int hw_net_mac (uint8_t mac[MAC_ADDR_LEN])
{
	CC3000_START;
	int ret = nvmem_get_mac_address(mac);
	CC3000_END;
	return ret;
}


void tm_net_initialize_dhcp_server (void)
{
  // Added by Hai Ta
  //
  // Network mask is assumed to be 255.255.255.0
  //

  uint8_t pucSubnetMask[4], pucIP_Addr[4], pucIP_DefaultGWAddr[4], pucDNS[4];

  pucDNS[0] = 0x0;
  pucDNS[1] = 0x0;
  pucDNS[2] = 0x0;
  pucDNS[3] = 0x0;

  pucSubnetMask[0] = 0;
  pucSubnetMask[1] = 0;
  pucSubnetMask[2] = 0;
  pucSubnetMask[3] = 0;
  pucIP_Addr[0] = 0;
  pucIP_Addr[1] = 0;
  pucIP_Addr[2] = 0;
  pucIP_Addr[3] = 0;
  // Use default gateway 192.168.1.1 here
  pucIP_DefaultGWAddr[0] = 0;
  pucIP_DefaultGWAddr[1] = 0;
  pucIP_DefaultGWAddr[2] = 0;
  pucIP_DefaultGWAddr[3] = 0;

  // In order for gethostbyname( ) to work, it requires DNS server to be configured prior to its usage
  // so I am gonna add full static

  // Netapp_Dhcp is used to configure the network interface, static or dynamic (DHCP).
  // In order to activate DHCP mode, aucIP, aucSubnetMask, aucDefaultGateway must be 0.The default mode of CC3000 is DHCP mode.
  netapp_dhcp((unsigned long *)&pucIP_Addr[0], (unsigned long *)&pucSubnetMask[0], (unsigned long *)&pucIP_DefaultGWAddr[0], (unsigned long *)&pucDNS[0]);
}

int hw_net_is_readable (int ulSocket)
{
	CC3000_START;
	fd_set readSet;        // Socket file descriptors we want to wake up for, using select()
	FD_ZERO(&readSet);
	FD_SET(ulSocket, &readSet);
	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int rcount = select( ulSocket+1, &readSet, (fd_set *) 0, (fd_set *) 0, &timeout );
	int flag = FD_ISSET(ulSocket, &readSet);
	CC3000_END;

	(void) rcount;
	return flag;
}

int hw_net_block_until_readable (int ulSocket, int timeout)
{
  while (1) {
    if (hw_net_is_readable(ulSocket)) {
      break;
    }
    if (timeout > 0 && --timeout == 0) {
      return -1;
    }
    hw_wait_ms(100);
  }
  return 0;
}

const unsigned long wifi_intervals[16] = {
	2000, 2000, 2000, 2000,
	2000, 2000, 2000, 2000,
	2000, 2000, 2000, 2000,
	2000, 2000, 2000, 2000
};

void hw_net_initialize (void)
{
	CC3000_START;
	SpiInit(4e6);
	hw_wait_us(10);

	wlan_init(CC3000_UsynchCallback, NULL, NULL, NULL, ReadWlanInterruptPin,
	WlanInterruptEnable, WlanInterruptDisable, WriteWlanPin);

	wlan_start(0);

	int r = wlan_ioctl_set_connection_policy(0, 0, 0);
	if (r != 0) {
		TM_DEBUG("Fail setting policy %i", r);
	}

	r = wlan_ioctl_set_scan_params(10000, 100, 100, 5, 0x7FF, -100, 0, 205, wifi_intervals);
	if (r != 0) {
		TM_DEBUG("Fail setting scan params %i", r);
	}

	r = wlan_ioctl_set_connection_policy(0, true, true);
	if (r != 0) {
		TM_DEBUG("Fail setting connection policy %i", r);
	}

	wlan_stop();
	hw_wait_ms(10);
	wlan_start(0);

//	tm_sleep_ms(100);
//	TM_COMMAND('w',"setting event mask\n");
	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE|HCI_EVNT_WLAN_UNSOL_INIT|HCI_EVNT_WLAN_ASYNC_PING_REPORT);
//	TM_COMMAND('w',"done setting event mask\n");

 	unsigned long aucDHCP = 14400;
	unsigned long aucARP = 3600;
	unsigned long aucKeepalive = 10;
	unsigned long aucInactivity = 0;
	if (netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity) != 0) {
		TM_DEBUG("Error setting inactivity timeout!");
	}

	unsigned char version[2];
	if (nvmem_read_sp_version(version)) {
		TM_ERR("Failed to read CC3000 firmware version.");
	} 

	memcpy(hw_cc_ver, version, 2);

	CC3000_END;
}

void hw_net_config(int should_connect_to_open_ap, int should_use_fast_connect, int auto_start){
	wlan_ioctl_set_connection_policy(should_connect_to_open_ap, should_use_fast_connect, auto_start);
}

void hw_net_smartconfig_initialize (void)
{
	CC3000_START;
	StartSmartConfig();
	CC3000_END;
}

void hw_net_disable (void)
{
	CC3000_START;
	wlan_stop();
	SpiDeInit();
	// clear out all wifi data
	memset(hw_wifi_ip, 0, sizeof hw_wifi_ip);
	memset(hw_wifi_ip, 0, sizeof hw_cc_ver);
	// reset connection
	ulCC3000Connected = 0;
	ulCC3000DHCP = 0;
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);
	CC3000_END;
}

int strcicmp(char const *a, char const *b)
{
	if (strlen(a) != strlen(b)) {
		return 1;
	}
	
    for (;; a++, b++) {
        int d = (tolower((unsigned char) *a) - tolower((unsigned char) *b));
        if (d != 0 || !*a)
            return d;
    }
}

//  WLAN_SEC_UNSEC,
//  WLAN_SEC_WEP (ASCII support only),
//  WLAN_SEC_WPA or WLAN_SEC_WPA2

static int error2count = 0;

__attribute__((weak)) void _cc3000_cb_acquire () { 
	// noop
}

__attribute__((weak)) void _cc3000_cb_error (int err) { 
	// noop
	(void) err;
}

int hw_net_connect (const char *security_type, const char *ssid, size_t ssid_len
	, const char *keys, size_t keys_len)
{
  CC3000_START;

  int security = WLAN_SEC_WPA2;
  char * security_print = "wpa2";
  if (keys_len == 0){
    security = WLAN_SEC_UNSEC;
    security_print = "unsecure";
  } else if (strcicmp(security_type, "wpa") == 0){
    security = WLAN_SEC_WPA;
    security_print = "wpa";
  } else if (strcicmp(security_type, "wep") == 0){
    security = WLAN_SEC_WEP;
    security_print = "wep";
  }

  TM_DEBUG("Attempting to connect with security type %s... ", security_print);
  wlan_ioctl_set_connection_policy(0, 1, 0);
  int connected = wlan_connect(security, (char *) ssid, ssid_len
  	, 0, (uint8_t *) keys, keys_len);

  if (connected != 0) {
    TM_DEBUG("Error #%d in connecting. Please try again.", connected);
  	// wlan_disconnect();
  	error2count = 0;
  	_cc3000_cb_error(connected);
  } else {
  	error2count = 0;
    TM_DEBUG("Acquiring IP address...");
  	_cc3000_cb_acquire();
  }
  CC3000_END;
  return connected;
}

int hw_net_disconnect (void)
{
	CC3000_START;
	int disconnect = wlan_disconnect();
	memset(hw_wifi_ip, 0, sizeof hw_wifi_ip);
	memset(hw_wifi_ip, 0, sizeof hw_cc_ver);
	// reset connection
	ulCC3000Connected = 0;
	ulCC3000DHCP = 0;

	CC3000_END;

	return disconnect;
}
