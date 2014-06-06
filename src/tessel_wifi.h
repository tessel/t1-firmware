// Copyright 2013 Technical Machine. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

// Tessel-specific wifi setup


#ifndef TESSEL_WIFI_H_
#define TESSEL_WIFI_H_

extern volatile int netconnected;
void tessel_wifi_enable ();
void tessel_wifi_disable ();
void tessel_wifi_smart_config ();

void tessel_wifi_init ();
void tessel_wifi_check(uint8_t output);
int tessel_wifi_connect(char * wifi_security, char * wifi_ssid, char* wifi_pass);
void tessel_wifi_fastconnect();

void _tessel_cc3000_irq_interrupt ();
void _cc3000_cb_animation_tick (size_t frame);

#endif /* TESSEL_WIFI_H_ */
