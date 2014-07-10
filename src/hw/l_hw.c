// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <stdlib.h>
#include <stdio.h>

#include "lpc18xx_gpio.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_libcfg.h"
#include "lpc18xx_timer.h"
#include "lpc18xx_cgu.h"

#include "hw.h"
#include "colony.h"
#include "l_hw.h"
#include "tessel.h"
#include "tm.h"

#include "audio-vs1053b.h"


#define ARG1 1

inline void stackDump(lua_State* L)
{
	int i;
	int top = lua_gettop(L);
	for (i = 1; i <= top; i++) { /* repeat for each level */
		int t = lua_type(L, i);
		switch (t) {

		case LUA_TSTRING: /* strings */
			printf("`%s'", lua_tostring(L, i));
			break;

		case LUA_TBOOLEAN: /* booleans */
			printf(lua_toboolean(L, i) ? "true" : "false");
			break;

		case LUA_TNUMBER: /* numbers */
			printf("%g", lua_tonumber(L, i));
			break;

		default: /* other values */
			printf("%s", lua_typename(L, t));
			break;
		}
		printf("  "); /* put a separator */
	}
	printf("\n"); /* end the listing */
}


// sleep

static int l_hw_sleep_us(lua_State* L)
{
	int us = (int)lua_tonumber(L, ARG1);

	hw_wait_us(us);
	return 0;
}


static int l_hw_sleep_ms(lua_State* L)
{
	int ms = (int)lua_tonumber(L, ARG1);

	hw_wait_ms(ms);
	return 0;
}


// spi

static int l_hw_spi_initialize(lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t clockspeed = (uint32_t)lua_tonumber(L, ARG1 + 1);
	uint8_t spimode = (uint8_t)lua_tonumber(L, ARG1 + 2);
	uint8_t cpol = (uint8_t)lua_tonumber(L, ARG1 + 3);
	uint8_t cpha = (uint8_t)lua_tonumber(L, ARG1 + 4);
	uint8_t framemode = (uint8_t)lua_tonumber(L, ARG1 + 5);
	hw_spi_initialize(port, clockspeed, spimode, cpol, cpha, framemode);
	return 0;
}


static int l_hw_spi_enable(lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	hw_spi_enable(port);
	return 0;
}


static int l_hw_spi_disable(lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	hw_spi_disable(port);
	return 0;
}


static int l_hw_spi_transfer(lua_State* L)
{

	// If the is currently a transfer underway, don't continue
	if (spi_async_status.rxRef > 0 || spi_async_status.txRef > 0) {
		// Push an error code onto the stack
		lua_pushnumber(L, -1);
		return 1;
	}
	// Grab the spi port number
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	// Create the tx/rx buffers
	size_t bufferLength = (size_t)lua_tonumber(L, ARG1 + 1);
	size_t buffer_check = 0;

	const uint8_t* txbuf = NULL;
	uint8_t* rxbuf = NULL;

	uint32_t txref = LUA_NOREF;

	txbuf = colony_toconstdata(L, ARG1 + 2, &buffer_check);
	// TODO - throw exception instead of throwing out txbuf
	if (bufferLength != buffer_check) {
		txbuf = NULL;
	}
	lua_pushvalue(L, ARG1 + 2);
	txref = luaL_ref(L, LUA_REGISTRYINDEX);


	uint32_t rxref = LUA_NOREF;
	rxbuf = colony_tobuffer(L, ARG1 + 3, &buffer_check);
	// TODO - throw exception instead of throwing out rxbuf
	if (bufferLength != buffer_check) {
		rxbuf = NULL;
	}
	lua_pushvalue(L, ARG1 + 3);
	rxref = luaL_ref(L, LUA_REGISTRYINDEX);


	size_t chunk_size = (size_t) lua_tonumber(L, ARG1 + 4);
	uint32_t repeat = (uint32_t) lua_tonumber(L, ARG1 + 5);
	uint8_t chip_select = (uint8_t) lua_tonumber(L, ARG1 + 6);
	// Begin the transfer
	hw_spi_transfer(port, bufferLength, txbuf, rxbuf, txref, rxref, chunk_size, repeat, chip_select, NULL);
	// Push a success code onto the stack
	lua_pushnumber(L, 0);
	return 1;
}

// i2c

static int l_hw_i2c_initialize (lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	hw_i2c_initialize(port);
	return 0;
}


static int l_hw_i2c_enable (lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint8_t mode = (uint8_t)lua_tonumber(L, ARG1 + 1);

	hw_i2c_enable(port, mode);
	return 0;
}

static int l_hw_i2c_disable (lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);

	hw_i2c_disable(port);
	return 0;
}

// I2C slave

static int l_hw_i2c_set_slave_addr (lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t address = (uint32_t)lua_tonumber(L, ARG1 + 1);

	hw_i2c_set_slave_addr(port, address);
	return 0;
}


static int l_hw_i2c_slave_transfer (lua_State* L)
{
	// int hw_i2c_slave_request_blocking(uint32_t port, const uint8_t *txbuf, size_t txbuf_len, uint8_t *rxbuf, size_t rxbuf_len)
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	// uint32_t address = (uint32_t) lua_tonumber(L, ARG1+1);
	size_t rxbuf_len = (size_t)lua_tonumber(L, ARG1 + 2);

	size_t txbuf_len = 0;
	const uint8_t* txbuf = colony_toconstdata(L, ARG1 + 1, &txbuf_len);

	uint8_t* rxbuf = colony_createbuffer(L, rxbuf_len);
	memset(rxbuf, 0, rxbuf_len);
	int res = hw_i2c_slave_transfer(port, txbuf, txbuf_len, rxbuf, rxbuf_len);

	lua_pushnumber(L, res);
	return 2;
}


static int l_hw_i2c_slave_send (lua_State* L)
{
	// port, length,
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	//  uint32_t address = (uint32_t) lua_tonumber(L, ARG1+1);

	size_t buf_len = 0;
	const uint8_t* txbuf = colony_toconstdata(L, ARG1 + 1, &buf_len);

	int res = hw_i2c_slave_send(port, txbuf, buf_len);
	lua_pushnumber(L, res);
	return 1;
}


static int l_hw_i2c_slave_receive (lua_State* L)
{
	// port, length,
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	size_t rxbuf_len = (size_t)lua_tonumber(L, ARG1 + 1);

	uint8_t* rxbuf = colony_createbuffer(L, rxbuf_len);
	memset(rxbuf, 0, rxbuf_len);

	int res = hw_i2c_slave_receive(port, rxbuf, rxbuf_len);

	lua_pushnumber(L, res);
	return 2;
}


// I2C master


static int l_hw_i2c_master_transfer (lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t address = (uint32_t)lua_tonumber(L, ARG1 + 1);
	size_t rxbuf_len = (size_t)lua_tonumber(L, ARG1 + 3);

	size_t txbuf_len = 0;
	const uint8_t* txbuf = colony_toconstdata(L, ARG1 + 2, &txbuf_len);

	uint8_t* rxbuf = colony_createbuffer(L, rxbuf_len);
	memset(rxbuf, 0, rxbuf_len);
	int res = hw_i2c_master_transfer(port, address, txbuf, txbuf_len, rxbuf, rxbuf_len);

	lua_pushnumber(L, res);
	lua_insert(L, -2);
	return 2;
}


static int l_hw_i2c_master_receive (lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t address = (uint32_t)lua_tonumber(L, ARG1 + 1);
	size_t rxbuf_len = (size_t)lua_tonumber(L, ARG1 + 2);

	uint8_t* rxbuf = colony_createbuffer(L, rxbuf_len);
	memset(rxbuf, 0, rxbuf_len);
	int res = hw_i2c_master_receive(port, address, rxbuf, rxbuf_len);

	lua_pushnumber(L, res);
	lua_insert(L, -2);
	return 2;
}


static int l_hw_i2c_master_send (lua_State* L)
{
	// port, address, length,
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t address = (uint32_t)lua_tonumber(L, ARG1 + 1);

	size_t buf_len = 0;
	const uint8_t* txbuf = colony_toconstdata(L, ARG1 + 2, &buf_len);

	int res = hw_i2c_master_send(port, address, txbuf, buf_len);
	lua_pushnumber(L, res);
	return 1;
}


// uart

static int l_hw_uart_enable(lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	hw_uart_enable(port);
	return 0;
}

static int l_hw_uart_disable(lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	hw_uart_disable(port);
	return 0;
}


static int l_hw_uart_initialize(lua_State* L)
{
	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t baudrate = (uint32_t)lua_tonumber(L, ARG1 + 1);
	UART_DATABIT_Type databits = (UART_DATABIT_Type)lua_tonumber(L, ARG1 + 2);
	UART_PARITY_Type parity = (UART_PARITY_Type)lua_tonumber(L, ARG1 + 3);
	UART_STOPBIT_Type stopbits = (UART_STOPBIT_Type)lua_tonumber(L, ARG1 + 4);
	hw_uart_initialize(port, baudrate, databits, parity, stopbits);
	return 0;
}


static int l_hw_uart_send(lua_State* L)
{

	uint32_t port = (uint32_t)lua_tonumber(L, ARG1);

	size_t buf_len = 0;
	const uint8_t* txbuf = colony_toconstdata(L, ARG1 + 1, &buf_len);

	uint8_t bytes = hw_uart_send(port, txbuf, buf_len);

	// Return number of bytes sent
	lua_pushnumber(L, bytes);
	return 1;
}


static int l_hw_digital_output(lua_State* L)
{
	uint32_t pin = (uint32_t)lua_tonumber(L, ARG1);
	hw_digital_output(pin);
	return 0;
}

static int l_hw_digital_input(lua_State* L)
{
	uint32_t pin = (uint32_t)lua_tonumber(L, ARG1);
	hw_digital_input(pin);
	return 0;
}



static int l_hw_analog_read(lua_State* L)
{
	uint32_t pin = (uint32_t)lua_tonumber(L, ARG1);

	lua_pushnumber(L, hw_analog_read(pin));

	return 1;
}


static int l_hw_analog_write(lua_State* L)
{
	uint32_t pin = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t level = (uint32_t)lua_tonumber(L, ARG1 + 1);

	hw_analog_write(pin, level);

	return 0;
}


static int l_hw_digital_read(lua_State* L)
{
	uint32_t pin = (uint32_t)lua_tonumber(L, ARG1);

	lua_pushnumber(L, hw_digital_read(pin));

	return 1;
}


static int l_hw_digital_write(lua_State* L)
{
	uint32_t pin = (uint32_t)lua_tonumber(L, ARG1);
	uint32_t level = (uint32_t)lua_tonumber(L, ARG1 + 1);

	hw_digital_write(pin, level);

	return 0;
}

static int l_hw_digital_set_mode(lua_State* L)
{
	uint8_t pin = (uint8_t)lua_tonumber(L, ARG1);
	uint8_t mode = (uint8_t)lua_tonumber(L, ARG1 + 1);
	
	lua_pushnumber(L, hw_digital_set_mode(pin, mode));

	return 1;
}

static int l_hw_digital_get_mode(lua_State* L)
{
	uint8_t pin = (uint8_t)lua_tonumber(L, ARG1);

	lua_pushnumber(L, hw_digital_get_mode(pin));

	return 1;
}


// interrupts

static int l_hw_interrupts_remaining(lua_State* L)
{
	lua_pushnumber(L, hw_interrupts_available());
	return 1;
}


static int l_hw_interrupt_watch(lua_State* L)
{
	int pin = (int)lua_tonumber(L, ARG1);
	int mode = (int)lua_tonumber(L, ARG1 + 1);
	int interrupt_index = (int)lua_tonumber(L, ARG1 + 2);
	lua_pushnumber(L, hw_interrupt_watch(pin, mode, interrupt_index, NULL));
	return 1;
}

static int l_hw_interrupt_unwatch(lua_State* L)
{
	int interrupt_index = (int)lua_tonumber(L, ARG1);
	int mode = (int)lua_tonumber(L, ARG1 + 1);
	lua_pushnumber(L, hw_interrupt_unwatch(interrupt_index, mode));
	return 1;
}


static int l_hw_interrupt_assignment_query(lua_State* L)
{
	int pin = (int)lua_tonumber(L, ARG1);
	lua_pushnumber(L, hw_interrupt_assignment_query(pin));
	return 1;
}


static int l_hw_acquire_available_interrupt(lua_State* L)
{
	lua_pushnumber(L, hw_interrupt_acquire());
	return 1;
}


// Old interrupt stuff:
// static int l_hw_interrupt_record (lua_State* L)
// {
//   int interrupt = (int) lua_tonumber(L, ARG1);
//   int pin = (int) lua_tonumber(L, ARG1+1);

//   hw_interrupt_record(interrupt, pin);
//   return 0;
// }


// static int l_hw_interrupt_record_lastrise (lua_State* L)
// {
//   int interrupt = (int) lua_tonumber(L, ARG1);

//   lua_pushnumber(L, hw_interrupt_record_lastrise(interrupt));
//   return 1;
// }


// static int l_hw_interrupt_record_lastfall (lua_State* L)
// {
//   int interrupt = (int) lua_tonumber(L, ARG1);

//   lua_pushnumber(L, hw_interrupt_record_lastfall(interrupt));
//   return 1;
// }


// highspeed

static int l_hw_highspeedsignal_initialize(lua_State* L)
{
	int speed = (int)lua_tonumber(L, ARG1);

	hw_highspeedsignal_initialize(speed);
	return 0;
}


static int l_hw_highspeedsignal_update(lua_State* L)
{
	// TODO
	(void)L;
	size_t size = 0;
	uint8_t* buf = (uint8_t*)""; //colony_buffer_ensure(L, 2, &size);

	hw_highspeedsignal_update(buf, size);
	return 0;
}


// net

static int l_hw_net_is_connected(lua_State* L)
{
	lua_pushnumber(L, hw_net_is_connected());
	return 1;
}


static int l_hw_net_local_ip(lua_State* L)
{
	uint32_t ret;
	memcpy(&ret, hw_wifi_ip, 4);
	lua_pushnumber(L, ret);
	return 1;
}


// device id
static int l_hw_device_id(lua_State* L)
{
	lua_pushstring(L, tessel_board_uuid());
	return 1;
}


// pwm
static int l_hw_pwm_port_period(lua_State *L) {
	uint32_t period = (uint32_t)lua_tonumber(L, ARG1);
	int ret = hw_pwm_port_period(period);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_hw_pwm_pin_pulsewidth(lua_State* L)
{
	size_t pin = (size_t)lua_tonumber(L, ARG1);
	uint32_t pulsewidth = (uint32_t)lua_tonumber(L, ARG1 + 1);

	int ret = hw_pwm_pin_pulsewidth(pin, pulsewidth);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_usb_send(lua_State* L)
{
	int tag = lua_tonumber(L, ARG1);
	size_t buf_len = 0;
	const uint8_t* txbuf = colony_tobuffer(L, ARG1+1, &buf_len);

	int r = hw_send_usb_msg(tag, txbuf, buf_len);
	lua_pushnumber(L, r);

	return 1;
}

// Module Shims

static int l_audio_play_buffer(lua_State* L) {
	uint8_t xcs = (uint8_t)lua_tonumber(L, ARG1);
	uint8_t dcs = (uint8_t)lua_tonumber(L, ARG1 + 1);
	uint8_t dreq = (uint8_t)lua_tonumber(L, ARG1 + 2);
	size_t buf_len = 0;
	const uint8_t* buf = colony_toconstdata(L, ARG1 + 3, &buf_len);

	int r;
	if (buf_len) {
		r = audio_play_buffer(xcs, dcs, dreq, buf, buf_len);
	}
	else {
		r = audio_resume_buffer();
	}

	lua_pushnumber(L, r);

	return 1;
}

static int l_audio_queue_buffer(lua_State* L) {
	uint8_t xcs = (uint8_t)lua_tonumber(L, ARG1);
	uint8_t dcs = (uint8_t)lua_tonumber(L, ARG1 + 1);
	uint8_t dreq = (uint8_t)lua_tonumber(L, ARG1 + 2);
	size_t buf_len = 0;
	const uint8_t* buf = colony_toconstdata(L, ARG1+3, &buf_len);

	int r = audio_queue_buffer(xcs, dcs, dreq, buf, buf_len);

	lua_pushnumber(L, r);

	return 1;
}

static int l_audio_stop_buffer(lua_State* L) {
	int r = audio_stop_buffer();
	lua_pushnumber(L, r);

	return 1;
}

static int l_audio_pause_buffer(lua_State* L) {
	int r = audio_pause_buffer();
	lua_pushnumber(L, r);

	return 1;
}

static int l_audio_get_state(lua_State* L) {
	lua_pushnumber(L, audio_get_state());
	return 1;
}

static int l_audio_start_recording(lua_State* L) {
	uint8_t xcs = (uint8_t)lua_tonumber(L, ARG1);
	// uint8_t dcs = (uint8_t)lua_tonumber(L, ARG1 + 1);
	uint8_t dreq = (uint8_t)lua_tonumber(L, ARG1 + 1);
	const uint8_t* dir_name = colony_toconstdata(L, ARG1 + 2, NULL);
	size_t buf_len;
	uint8_t *buf = colony_tobuffer(L, ARG1+3, &buf_len);
	uint32_t buf_ref = luaL_ref(L, LUA_REGISTRYINDEX);
	
	int8_t r = audio_start_recording(xcs, dreq, (char *)dir_name, buf, buf_len, buf_ref);

	lua_pushnumber(L, r);
	return 1;
}

static int l_audio_stop_recording(lua_State* L) {
	int16_t r = audio_stop_recording(true);

	lua_pushnumber(L, r);
	return 1;
}

/**
 * NTP
 */


// synchronous function for getting the current time.
// this is a poor design.

uint32_t tm__sync_gethostbyname (const char *domain);

static int l_clocksync (lua_State* L) {
	if (!hw_net_is_connected()) {
		return 0;
	}

	int sock = tm_udp_open();
	if (sock < 0) {
		return 0;
	}

	// RFC 2030 -> LI = 0 (no warning, 2 bits), VN = 3 (IPv4 only, 3 bits), Mode = 3 (Client Mode, 3 bits) -> 1 byte
	// -> rtol(LI, 6) ^ rotl(VN, 3) ^ rotl(Mode, 0)
	// -> = 0x00 ^ 0x18 ^ 0x03
	uint8_t ntpData[48] = { 0 };
	ntpData[0] = 0x1B;
	for (int i = 1; i < 48; i++) {
		ntpData[i] = 0;
	}

	uint32_t ip = tm__sync_gethostbyname("pool.ntp.org");
	tm_udp_listen(sock, 7000);
	tm_udp_send(sock, (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip >> 0) & 0xFF, 123, ntpData, 48);
	uint8_t buf[256] = { 0 };
	uint32_t retip;
	int size = tm_udp_receive(sock, buf, 256, &retip);
	(void) size;
	tm_udp_close(sock);

	// Offset to get to the "Transmit Timestamp" field (time at which the reply
	// departed the server for the client, in 64-bit timestamp format."
	long long intpart = 0, fractpart = 0;
	int offsetTransmitTime = 40, i = 0;

	// Get the seconds part
	for (i = 0; i <= 3; i++) {
		intpart = 256 * intpart + buf[offsetTransmitTime + i];
	}

	// Get the seconds fraction
	for (i = 4; i <= 7; i++) {
		fractpart = 256 * fractpart + buf[offsetTransmitTime + i];
	}

	long long epochto1900 = -2208988800000;
	long long milliseconds = (intpart * 1000 + (fractpart * 1000) / 0x100000000L) + epochto1900;
	lua_pushnumber(L, milliseconds);
	return 1;
}


#ifdef __cplusplus
extern "C" {
#endif

#define luaL_setfieldnumber(L, str, num) \
	lua_pushnumber(L, num);          \
	lua_setfield(L, -2, str);

LUALIB_API int luaopen_hw(lua_State* L)
{
	luaL_reg regs[] = {

		// spi
		{ "spi_initialize", l_hw_spi_initialize },
		{ "spi_enable", l_hw_spi_enable },
		{ "spi_disable", l_hw_spi_disable },
		{ "spi_transfer", l_hw_spi_transfer },

		// i2c
		{ "i2c_initialize", l_hw_i2c_initialize },
		{ "i2c_set_slave_addr", l_hw_i2c_set_slave_addr },
		{ "i2c_enable", l_hw_i2c_enable },
		{ "i2c_disable", l_hw_i2c_disable },
		{ "i2c_master_transfer", l_hw_i2c_master_transfer },
		{ "i2c_master_send", l_hw_i2c_master_send },
		{ "i2c_master_receive", l_hw_i2c_master_receive },
		{ "i2c_slave_transfer", l_hw_i2c_slave_transfer },
		{ "i2c_slave_send", l_hw_i2c_slave_send },
		{ "i2c_slave_receive", l_hw_i2c_slave_receive },

		//uart
		{ "uart_enable", l_hw_uart_enable },
		{ "uart_disable", l_hw_uart_disable },
		{ "uart_initialize", l_hw_uart_initialize },
		{ "uart_send", l_hw_uart_send },

		// sleep
		{ "sleep_us", l_hw_sleep_us },
		{ "sleep_ms", l_hw_sleep_ms },

		// gpio / leds
		{ "digital_output", l_hw_digital_output },
		{ "digital_input", l_hw_digital_input },
		{ "digital_write", l_hw_digital_write },
		{ "digital_read", l_hw_digital_read },
		{ "digital_get_mode", l_hw_digital_get_mode},
		{ "digital_set_mode", l_hw_digital_set_mode},
		{ "analog_write", l_hw_analog_write },
		{ "analog_read", l_hw_analog_read },

		// external gpio interrupts
		{ "interrupts_remaining", l_hw_interrupts_remaining },
		{ "interrupt_watch", l_hw_interrupt_watch },
		{ "interrupt_unwatch", l_hw_interrupt_unwatch },
		{ "interrupt_assignment_query", l_hw_interrupt_assignment_query },
		{ "acquire_available_interrupt", l_hw_acquire_available_interrupt },

		// buffer
		{ "highspeedsignal_initialize", l_hw_highspeedsignal_initialize },
		{ "highspeedsignal_update", l_hw_highspeedsignal_update },

		// net
		{ "is_connected", l_hw_net_is_connected },
		{ "local_ip", l_hw_net_local_ip },

		// device id
		{ "device_id", l_hw_device_id },

		// pwm
		{ "pwm_port_period", l_hw_pwm_port_period },
		{ "pwm_pin_pulsewidth", l_hw_pwm_pin_pulsewidth },

		// usb
		{ "usb_send", l_usb_send },

		// module shims
		// audio
		{ "audio_play_buffer", l_audio_play_buffer },
		{ "audio_queue_buffer", l_audio_queue_buffer },
		{ "audio_stop_buffer", l_audio_stop_buffer },
		{ "audio_pause_buffer", l_audio_pause_buffer },
		{ "audio_get_state", l_audio_get_state },
		{ "audio_start_recording", l_audio_start_recording },
		{ "audio_stop_recording", l_audio_stop_recording },

		// clock sync
		{ "clocksync", l_clocksync },

		// End of array (must be last)
		{ NULL, NULL }
	};

	// Create object.
	lua_newtable(L);
	luaL_register(L, NULL, regs);

	luaL_setfieldnumber(L, "SPI_0", TESSEL_SPI_0);
	luaL_setfieldnumber(L, "SPI_1", TESSEL_SPI_1);
	luaL_setfieldnumber(L, "SPI_LOW", HW_SPI_LOW);
	luaL_setfieldnumber(L, "SPI_HIGH", HW_SPI_HIGH);
	luaL_setfieldnumber(L, "SPI_FIRST", HW_SPI_FIRST);
	luaL_setfieldnumber(L, "SPI_SECOND", HW_SPI_SECOND);
	luaL_setfieldnumber(L, "SPI_SLAVE_MODE", HW_SPI_SLAVE);
	luaL_setfieldnumber(L, "SPI_MASTER_MODE", HW_SPI_MASTER);
	luaL_setfieldnumber(L, "I2C_1", TM_I2C_1);
	luaL_setfieldnumber(L, "I2C_0", TM_I2C_0);
	luaL_setfieldnumber(L, "I2C_MASTER", I2C_MASTER_MODE);
	luaL_setfieldnumber(L, "I2C_SLAVE", I2C_SLAVE_MODE);
	luaL_setfieldnumber(L, "I2C_GENERAL", I2C_GENERAL_MODE);
	luaL_setfieldnumber(L, "UART_0", (uint32_t)UART0);
	luaL_setfieldnumber(L, "UART_2", (uint32_t)UART2);
	luaL_setfieldnumber(L, "UART_3", (uint32_t)UART3);
	luaL_setfieldnumber(L, "UART_SW_0", (uint32_t)UART_SW_0);
	luaL_setfieldnumber(L, "UART_SW_1", (uint32_t)UART_SW_1);
	luaL_setfieldnumber(L, "UART_PARITY_NONE", (uint32_t)UART_PARITY_NONE);
	luaL_setfieldnumber(L, "UART_PARITY_ODD", (uint32_t)UART_PARITY_ODD);
	luaL_setfieldnumber(L, "UART_PARITY_EVEN", (uint32_t)UART_PARITY_EVEN);
	luaL_setfieldnumber(L, "UART_PARITY_ONE_STICK", (uint32_t)UART_PARITY_SP_1);
	luaL_setfieldnumber(L, "UART_PARITY_ZERO_STICK", (uint32_t)UART_PARITY_SP_0);
	luaL_setfieldnumber(L, "UART_DATABIT_5", (uint32_t)UART_DATABIT_5);
	luaL_setfieldnumber(L, "UART_DATABIT_6", (uint32_t)UART_DATABIT_6);
	luaL_setfieldnumber(L, "UART_DATABIT_7", (uint32_t)UART_DATABIT_7);
	luaL_setfieldnumber(L, "UART_DATABIT_8", (uint32_t)UART_DATABIT_8);
	luaL_setfieldnumber(L, "UART_STOPBIT_1", (uint32_t)UART_STOPBIT_1);
	luaL_setfieldnumber(L, "UART_STOPBIT_2", (uint32_t)UART_STOPBIT_2);
	luaL_setfieldnumber(L, "PIN_PULLUP", MD_PUP);
	luaL_setfieldnumber(L, "PIN_PULLDOWN", MD_PDN);
	luaL_setfieldnumber(L, "PIN_DEFAULT", MD_PLN|MD_EZI|MD_ZI);

	luaL_setfieldnumber(L, "PIN_A_G1", A_G1);
	luaL_setfieldnumber(L, "PIN_A_G2", A_G2);
	luaL_setfieldnumber(L, "PIN_A_G3", A_G3);
	luaL_setfieldnumber(L, "PIN_B_G1", B_G1);
	luaL_setfieldnumber(L, "PIN_B_G2", B_G2);
	luaL_setfieldnumber(L, "PIN_B_G3", B_G3);
	luaL_setfieldnumber(L, "PIN_C_G1", C_G1);
	luaL_setfieldnumber(L, "PIN_C_G2", C_G2);
	luaL_setfieldnumber(L, "PIN_C_G3", C_G3);
	luaL_setfieldnumber(L, "PIN_D_G1", D_G1);
	luaL_setfieldnumber(L, "PIN_D_G2", D_G2);
	luaL_setfieldnumber(L, "PIN_D_G3", D_G3);
	luaL_setfieldnumber(L, "PIN_E_G1", E_G1);
	luaL_setfieldnumber(L, "PIN_E_G2", E_G2);
	luaL_setfieldnumber(L, "PIN_E_G3", E_G3);
	luaL_setfieldnumber(L, "PIN_E_G4", E_G4);
	luaL_setfieldnumber(L, "PIN_E_G5", E_G5);
	luaL_setfieldnumber(L, "PIN_E_G6", E_G6);
	luaL_setfieldnumber(L, "PIN_E_A1", E_A1);
	luaL_setfieldnumber(L, "PIN_E_A2", E_A2);
	luaL_setfieldnumber(L, "PIN_E_A3", E_A3);
	luaL_setfieldnumber(L, "PIN_E_A4", E_A4);
	luaL_setfieldnumber(L, "PIN_E_A5", E_A5);
	luaL_setfieldnumber(L, "PIN_E_A6", E_A6);
	luaL_setfieldnumber(L, "PIN_LED1", LED1);
	luaL_setfieldnumber(L, "PIN_LED2", LED2);
	luaL_setfieldnumber(L, "PIN_WIFI_CONN_LED", CC3K_CONN_LED);
	luaL_setfieldnumber(L, "PIN_WIFI_ERR_LED", CC3K_ERR_LED);
	luaL_setfieldnumber(L, "BUTTON", BTN1);
	luaL_setfieldnumber(L, "LOW", HW_LOW);
	luaL_setfieldnumber(L, "HIGH", HW_HIGH);

	return 1;
}


#ifdef __cplusplus
}
#endif
