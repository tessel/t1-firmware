/*
 * test.c
 *
 *  Created on: Aug 8, 2013
 *      Author: tim
 */

#include <stdint.h>
#include <stdlib.h>

#include "hw.h"
#include "tm.h"
#include "tessel.h"
#include "tm.h"
#include "sdram_init.h"


/**
 * LEDs
 */

void test_leds (void)
{
	hw_digital_output(LED1);
	hw_digital_output(LED2);
	volatile int i = 0;
	while (1) {
		hw_digital_write(LED1, 0);
		hw_digital_write(LED2, 1);
		for (i = 0; i < 1000000; i++) { }
		hw_digital_write(LED1, 1);
		hw_digital_write(LED2, 0);
		for (i = 0; i < 1000000; i++) { }
	}
}


/**
 * GPIOs
 */

void test_gpios (void)
{
	hw_digital_output(A_G1);
	hw_digital_output(A_G2);
	hw_digital_output(A_G3);
	hw_digital_write(A_G1, 1);
	hw_digital_write(A_G2, 1);
	hw_digital_write(A_G3, 1);

	hw_digital_output(B_G1);
	hw_digital_output(B_G2);
	hw_digital_output(B_G3);
	hw_digital_write(B_G1, 1);
	hw_digital_write(B_G2, 1);
	hw_digital_write(B_G3, 1);

	hw_digital_output(C_G1);
	hw_digital_output(C_G2);
	hw_digital_output(C_G3);
	hw_digital_write(C_G1, 1);
	hw_digital_write(C_G2, 1);
	hw_digital_write(C_G3, 1);

	hw_digital_output(D_G1);
	hw_digital_output(D_G2);
	hw_digital_output(D_G3);
	hw_digital_write(D_G1, 1);
	hw_digital_write(D_G2, 1);
	hw_digital_write(D_G3, 1);

	hw_digital_output(E_G1);
	hw_digital_output(E_G2);
	hw_digital_output(E_G3);
	hw_digital_output(E_G4);
	hw_digital_output(E_G5);
	hw_digital_output(E_G6);
	hw_digital_write(E_G1, 1);
	hw_digital_write(E_G2, 1);
	hw_digital_write(E_G3, 1);
	hw_digital_write(E_G4, 1);
	hw_digital_write(E_G5, 1);
	hw_digital_write(E_G6, 1);

	hw_digital_output(E_A4);
	hw_digital_output(E_A5);
	hw_digital_write(E_A4, 1);
	hw_digital_write(E_A5, 1);
}


/**
 * UDP
 */

void test_udp_send (void)
{
	hw_net_initialize();
	hw_net_block_until_dhcp();

	// Send UDP packet
	while (1) {
		long socket = tm_udp_open();
		printf("Sending packet...\n");
		tm_udp_send(socket, 10, 1, 90, 138, 4444, (uint8_t *) "haha", 4);
		socket = tm_udp_close(socket);
		printf("Sent UDP packet.\n");
	}
}

//printf("Listening on 3333...");
//long socket = tm_net_udp_open_socket();
//tm_net_udp_listen(socket, 3333);
//while (1) {
//	while (!tm_net_is_readable(socket)) { }
//	printf("Readable.\n");
//	uint8_t buf[256];
//	sockaddr from;
//	socklen_t from_len;
//	int read = tm_net_udp_receive(socket, buf, sizeof(buf), &from, &from_len);
//	printf("Received %d bytes.\n", read);
//}
//socket = tm_net_udp_close_socket(socket);

#include "socket.h"

void test_udp_receive (void)
{
	hw_net_initialize();
	hw_net_block_until_dhcp();

	hw_digital_write(CC3K_CONN_LED, 1);

	uint8_t buf[255];

	// Receive UDP packets
	long socket = tm_udp_open();
	tm_udp_listen(socket, 4444);

	while (1) {
		hw_net_block_until_readable(socket, 0);
		uint32_t from = 0;
		int received = tm_udp_receive(socket, buf, sizeof(buf), &from);
		if (received > 0) {
			hw_digital_write(LED1, 1);
			hw_wait_ms(200);
			hw_digital_write(LED1, 0);
		}
	}
}

/*
 * I2C
 */

void test_i2c (char port)
{
  uint32_t i2c = (uint32_t) LPC_I2C1;
  if (port == 'c' || port == 'd'){
    i2c = (uint32_t) LPC_I2C0;
  }
  
  hw_i2c_initialize(i2c);
  hw_i2c_enable(i2c, I2C_MASTER);
  // write something
  uint32_t addr = 0x48;
  uint8_t buff[5] = {10, 11, 12, 13, 14};
  hw_i2c_master_send(i2c, addr, buff, 5);
  hw_wait_ms(100);
}

// here's some i2c slave/master stuff
void test_i2c_master_slave (uint8_t mode)
{
  #define I2CDEV_S_ADDR (0xDD>>1)

	uint32_t port = (uint32_t) LPC_I2C0;

//   i2c test
  if (mode == I2C_MASTER) {
    while (1){
      hw_i2c_initialize(port);
      hw_i2c_enable(port, I2C_MASTER);
      uint8_t txbuff[5] = {0xaa, 0xbb, 0x03, 0x04, 0x05};
      hw_i2c_master_send(port, I2CDEV_S_ADDR, txbuff, 5);
      for (int i = 0; i<100000; i++){

      }
      TM_DEBUG("sending i2c");
    }
  }
//   listen test

  if (mode == I2C_SLAVE) {
    while (1) {
      hw_i2c_initialize(port);
      hw_i2c_enable(port, I2C_SLAVE);
      hw_i2c_set_slave_addr(port, I2CDEV_S_ADDR);
    //  uint32_t addr = 0xDD;
      uint8_t rxbuff[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
      hw_i2c_slave_receive(port, rxbuff, 5);
//      TM_DEBUG("recieved");
      TM_DEBUG("received %d, %d, %d, %d, %d", rxbuff[0], rxbuff[1], rxbuff[2], rxbuff[3], rxbuff[4]);
      for (int i = 0; i<100000; i++){

      }
    }
  }
}

/*
 * SPI (external)
 */

void test_spi (char port)
{
  int pin = A_G1;
  if (port == 'b'){
    pin = B_G1;
  } else if (port == 'c'){
    pin = C_G1;
  } else if (port == 'd'){
    pin = D_G1;
  } else if (port == 'g'){
    pin = E_G1;
  }
  hw_digital_output(pin);
  hw_digital_write(pin, 0);
  hw_spi_initialize(TESSEL_SPI_0, 1000000, HW_SPI_MASTER, 0, 0, HW_SPI_FRAME_NORMAL);
  uint8_t buff[5] = {10, 11, 12, 13, 14};
  hw_spi_send (TESSEL_SPI_0, buff, 5);
  hw_digital_write(pin, 1);

  hw_wait_ms(100);
}


/*
 * UART
 */

void test_uart (char port)
{
  uint32_t baudrate = 9600;
  uint8_t buff[5] = {10, 11, 12, 13, 14};
  if (port == 'c' || port == 'g'){
    // do the software uart test
    sw_uart_test_c();

    // only have port c right now, will add gpio port later
    return;
  }

  // otherwise its a hardware uart test
  uint32_t uart = UART2;
  if (port == 'a'){
    uart = UART3;
  } else if(port == 'b'){
    uart = UART2;
  } else if (port == 'd'){
    uart = UART0;
  }

  hw_uart_initialize(uart, baudrate, UART_DATABIT_8, UART_PARITY_NONE, UART_STOPBIT_1);
  hw_uart_send(uart, buff, 5);
  hw_wait_ms(100);
}


/**
 * ADC
 */

#define DIR_IN				0 //Direction out

void test_gpio_read (void)
{
	hw_digital_input(A_G1);
	hw_digital_input(A_G2);
	hw_digital_input(A_G3);

	hw_digital_input(B_G1);
	hw_digital_input(B_G2);
	hw_digital_input(B_G3);

	hw_digital_input(C_G1);
	hw_digital_input(C_G2);
	hw_digital_input(C_G3);

	hw_digital_input(D_G1);
	hw_digital_input(D_G2);
	hw_digital_input(D_G3);

	hw_digital_input(E_G1);
	hw_digital_input(E_G2);
	hw_digital_input(E_G3);
	hw_digital_input(E_G4);
	hw_digital_input(E_G5);
	hw_digital_input(E_G6);

	// test gpios
	while (1) {
		printf("A");
		printf(hw_digital_read(A_G1) ? "X" : ".");
		printf(hw_digital_read(A_G2) ? "X" : ".");
		printf(hw_digital_read(A_G3) ? "X" : ".");
		printf(" B");
		printf(hw_digital_read(B_G1) ? "X" : ".");
		printf(hw_digital_read(B_G2) ? "X" : ".");
		printf(hw_digital_read(B_G3) ? "X" : ".");
		printf(" C");
		printf(hw_digital_read(C_G1) ? "X" : ".");
		printf(hw_digital_read(C_G2) ? "X" : ".");
		printf(hw_digital_read(C_G3) ? "X" : ".");
		printf(" D");
		printf(hw_digital_read(D_G1) ? "X" : ".");
		printf(hw_digital_read(D_G2) ? "X" : ".");
		printf(hw_digital_read(D_G3) ? "X" : ".");
		printf(" E");
		printf(hw_digital_read(E_G1) ? "X" : ".");
		printf(hw_digital_read(E_G2) ? "X" : ".");
		printf(hw_digital_read(E_G3) ? "X" : ".");
		printf(hw_digital_read(E_G4) ? "X" : ".");
		printf(hw_digital_read(E_G5) ? "X" : ".");
		printf(hw_digital_read(E_G6) ? "X" : ".");
		printf("\n");

		for (int i = 0; i < 1000000; i++) { }
	}
}

void test_adc (void)
{
	// only analog
	uint32_t analogRead0 = 0;
	uint32_t analogRead1 = 0;
	uint32_t analogRead2 = 0;

	// also gpios
	uint32_t analogRead3 = 0;
	uint32_t analogRead4 = 0;
	uint32_t analogRead5 = 0;
	while (1) {
		analogRead0 = hw_analog_read(E_A1);
		TM_DEBUG("A1: %u", (unsigned int) analogRead0);

		analogRead1 = hw_analog_read(E_A2);
		TM_DEBUG("A2: %u", (unsigned int) analogRead1);

		analogRead2 = hw_analog_read(E_A3);
		TM_DEBUG("A3: %u", (unsigned int) analogRead2);

		analogRead3 = hw_analog_read(E_A4);
		TM_DEBUG("A4: %u", (unsigned int) analogRead3);

		analogRead4 = hw_analog_read(E_A5);
		TM_DEBUG("A5: %u", (unsigned int) analogRead4);

		analogRead5 = hw_analog_read(E_A6);
		TM_DEBUG("A6: %u", (unsigned int) analogRead5);
		
		for (int i = 0; i < 1000000; i++) { }
	}
}

/**
 * DAC
 */

void test_dac (void)
{
  TM_DEBUG("starting dac test");
	int a = 0;
	while (1) {
		hw_analog_write(E_A4, a);
		//for (i = 0; i < 100000; i++) { }
		a += 255;
		if (a > 1024) {
			a = 0;
		}
		hw_wait_ms(300);
	}
}


/**
 * PWM
 */

void test_pwm (void)
{
	int a = 0;
	while (1) {
		hw_pwm_enable(E_G4, 0, 255, a);
		//for (i = 0; i < 100000; i++) { }
		a += 1;
		if (a > 255) {
			a = 0;
		}
	}
}


/**
 * SDRAM
 */

//#define SDRAM_SIZE_BYTES 512*1024//16*1024*1024
#define TEST_SDRAM_BASE SDRAM_ADDR_BASE

#define TEST_SDRAM_OFFSET 27*1024*1024//40*1024*1024
#define TEST_SDRAM_MIN 0//512*1024
#define TEST_SDRAM_MAX (5*1024*1024)
#define TEST_SDRAM_REPEAT 1

// writes 1 to x to the sdram
// returns 0 if it worked, -1 if it failed
int test_sdram_1 (void)
{
	int worked = 0;
	volatile uint32_t *ramdata;
	uint32_t temp = 0;
	uint32_t tempCorrect = 0;
	uint32_t i;
	uint32_t a;
	ramdata = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);

	for (a = 0; a<TEST_SDRAM_REPEAT; a++){
		for(i=TEST_SDRAM_MIN;i<TEST_SDRAM_MAX;i++){
			*ramdata = i;
			//*temp = *ramdata;
			ramdata++;
		}
	}

//	printf("Check RAM...\r\n");
	ramdata = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
	for (a = 0; a<TEST_SDRAM_REPEAT; a++){
		for(i=TEST_SDRAM_MIN;i<TEST_SDRAM_MAX;i++){
			if(*ramdata != i) {
				temp = *ramdata;
	//			while(1);
				worked = -1;
			} else {
				tempCorrect = *ramdata;
			}
			ramdata++;
		}
	}

//	printf("RAM Check Finish...\r\n");

//	printf("Clear RAM content...\r\n");
	ramdata = (volatile uint32_t *)TEST_SDRAM_BASE;
	for(i=TEST_SDRAM_MIN;i<TEST_SDRAM_MAX;i++){
		*ramdata = 0;
		ramdata++;
	}

	(void) temp;
	(void) tempCorrect;
	return worked;
}

// writes 1, 0, and constants to the sdram
// returns 0 if it worked, -1 if it failed
int test_sdram_2 (void)
{
	int worked = 0;
	// uint32_t *ramdata;
	// uint32_t temp;
	// uint32_t tempCorrect = 0;
	uint32_t i;
	// uint32_t a;
	uint32_t value;
	volatile uint32_t *pData;
	// Running "1" test
	value = 0x1;
	uint32_t test_value;
	// uint32_t test_value2;

	while (value != 0)
	{
		// Write
		pData = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
		for (i = 0; i < (TEST_SDRAM_MAX); i++)
		{
			*pData++ = value;
		}
		// Verify
		pData = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
		for (i = 0; i < (TEST_SDRAM_MAX); i++)
		{
			test_value = *pData++;
			if (test_value != value)
			{
				//while (1);      // catch the error
				test_value = *pData;
				worked = -1;
			}
		}

		value <<= 1;    // next bit
	}

	// Running "0" test
	value = ~0x1;

	while (value != ~((unsigned int) 0))
	{
		// Write
		pData = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
		for (i = 0; i < (TEST_SDRAM_MAX ); i++)
		{
			*pData++ = value;
		}
		// Verify
		pData = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
		for (i = 0; i < (TEST_SDRAM_MAX); i++)
		{
			test_value = *pData++;
//			test_value = *pData;
			if (test_value != value)
			{
//				while (1);      // catch the error
				test_value = *pData;
				worked = -1;
			}
		}

		value = (value << 1) | 0x1; // next bit
	}

	// "Const" test
	value = 0x55555555;

	// Write
	pData = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
	for (i = 0; i < (TEST_SDRAM_MAX); i++)
	{
		*pData++ = value;
		*pData++ = ~value;
	}
	// Verify
	pData = (volatile uint32_t *)(TEST_SDRAM_BASE + TEST_SDRAM_OFFSET);
	for (i = 0; i < (TEST_SDRAM_MAX); i++)
	{
//		test_value = *pData++;
//		test_value2 = *pData++;
		if (*pData++ != value || *pData++ != ~value)
		{
//			while (1);      // catch the error
			test_value = *pData;
			worked = -1;
		}
	}

	return worked;
}


/**
 * Neopixel / High speed signaling
 */

#include "hw.h"
#include "math.h"

#define COLOR_BIT(N, i) (N & (0x1 << (i)) ? 0b110 : 0b100)

static void test_neopixel_add_color (uint8_t *buf, int count, uint32_t rgb)
{
	uint32_t grb = (rgb & 0xFF) | ((rgb >> 8) & (0xFF00)) | ((rgb << 8) & (0xFF0000));

	int j = (count + 2) * 9;
	int k = 23;

	// 23 - 16
	buf[j++] |= (COLOR_BIT(grb, k) << 5) | (COLOR_BIT(grb, k - 1) << 2) | (COLOR_BIT(grb, k - 2) >> 1);
	buf[j++] |= (COLOR_BIT(grb, k - 2) << 7) | (COLOR_BIT(grb, k - 3) << 4) | (COLOR_BIT(grb, k - 4) << 1) | (COLOR_BIT(grb, k - 5) >> 2);
	buf[j++] |= (COLOR_BIT(grb, k - 5) << 6) | (COLOR_BIT(grb, k - 6) << 3) | (COLOR_BIT(grb, k - 7) << 0);
	// volatile int a = buf[j - 3], b = buf[j - 2], c = buf[j - 1];
	k -= 8;

	// 15 - 8
	buf[j++] |= (COLOR_BIT(grb, k) << 5) | (COLOR_BIT(grb, k - 1) << 2) | (COLOR_BIT(grb, k - 2) >> 1);
	buf[j++] |= (COLOR_BIT(grb, k - 2) << 7) | (COLOR_BIT(grb, k - 3) << 4) | (COLOR_BIT(grb, k - 4) << 1) | (COLOR_BIT(grb, k - 5) >> 2);
	buf[j++] |= (COLOR_BIT(grb, k - 5) << 6) | (COLOR_BIT(grb, k - 6) << 3) | (COLOR_BIT(grb, k - 7) << 0);
	k -= 8;

	// 7 - 0
	buf[j++] |= (COLOR_BIT(grb, k) << 5) | (COLOR_BIT(grb, k - 1) << 2) | (COLOR_BIT(grb, k - 2) >> 1);
	buf[j++] |= (COLOR_BIT(grb, k - 2) << 7) | (COLOR_BIT(grb, k - 3) << 4) | (COLOR_BIT(grb, k - 4) << 1) | (COLOR_BIT(grb, k - 5) >> 2);
	buf[j++] |= (COLOR_BIT(grb, k - 5) << 6) | (COLOR_BIT(grb, k - 6) << 3) | (COLOR_BIT(grb, k - 7) << 0);
}

static void test_neopixel_set_color (uint8_t *buf, int count, uint32_t rgb)
{
	uint32_t grb = (rgb & 0xFF) | ((rgb >> 8) & (0xFF00)) | ((rgb << 8) & (0xFF0000));

	int j = (count + 2) * 9;
	int k = 23;

	// 23 - 16
	buf[j++] = (COLOR_BIT(grb, k) << 5) | (COLOR_BIT(grb, k - 1) << 2) | (COLOR_BIT(grb, k - 2) >> 1);
	buf[j++] = (COLOR_BIT(grb, k - 2) << 7) | (COLOR_BIT(grb, k - 3) << 4) | (COLOR_BIT(grb, k - 4) << 1) | (COLOR_BIT(grb, k - 5) >> 2);
	buf[j++] = (COLOR_BIT(grb, k - 5) << 6) | (COLOR_BIT(grb, k - 6) << 3) | (COLOR_BIT(grb, k - 7) << 0);
	// volatile int a = buf[j - 3], b = buf[j - 2], c = buf[j - 1];
	k -= 8;

	// 15 - 8
	buf[j++] = (COLOR_BIT(grb, k) << 5) | (COLOR_BIT(grb, k - 1) << 2) | (COLOR_BIT(grb, k - 2) >> 1);
	buf[j++] = (COLOR_BIT(grb, k - 2) << 7) | (COLOR_BIT(grb, k - 3) << 4) | (COLOR_BIT(grb, k - 4) << 1) | (COLOR_BIT(grb, k - 5) >> 2);
	buf[j++] = (COLOR_BIT(grb, k - 5) << 6) | (COLOR_BIT(grb, k - 6) << 3) | (COLOR_BIT(grb, k - 7) << 0);
	k -= 8;

	// 7 - 0
	buf[j++] = (COLOR_BIT(grb, k) << 5) | (COLOR_BIT(grb, k - 1) << 2) | (COLOR_BIT(grb, k - 2) >> 1);
	buf[j++] = (COLOR_BIT(grb, k - 2) << 7) | (COLOR_BIT(grb, k - 3) << 4) | (COLOR_BIT(grb, k - 4) << 1) | (COLOR_BIT(grb, k - 5) >> 2);
	buf[j++] = (COLOR_BIT(grb, k - 5) << 6) | (COLOR_BIT(grb, k - 6) << 3) | (COLOR_BIT(grb, k - 7) << 0);
}

void test_neopixel ()
{
	int lights = 162;

	// uint32_t rgb = 0x00;
	int ridx = 0, rdir = -1, rlen = 3, rdist = 0;
	int gidx = 16, gdir = 1, glen = 10, gdist = 8;
	int bidx = 32, bdir = 1, blen = 20, bdist = 16;
	int frame = 0;

	hw_highspeedsignal_initialize(2400000);
	size_t lightbuf_len = (lights + 2) * 9;
	uint8_t *lightbuf = calloc(1, lightbuf_len);

	int normalcolor = 0x010101;

	for (int i = 0; i < lights; i++) {
		test_neopixel_set_color(lightbuf, i, normalcolor);
	}

	while (1) {

		for (int j = 0; j < blen; j++) {
			test_neopixel_set_color(lightbuf, (bidx + j) % lights, normalcolor);
		}
		for (int j = 0; j < glen; j++) {
			test_neopixel_set_color(lightbuf, (gidx + j) % lights, normalcolor);
		}
		for (int j = 0; j < rlen; j++) {
			test_neopixel_set_color(lightbuf, (ridx + j) % lights, normalcolor);
		}

		if (ridx == 0) {
			ridx = lights;
		}
		ridx += rdir;
//		rdir = ridx >= lights - 1 ? -1 : ridx == 0 ? 1 : rdir;
		if (frame % 2) {
			gidx += gdir;
		}
//		gdir = gidx >= lights - 1 ? -1 : gidx == 0 ? 1 : gdir;
		if (frame % 3) {
			bidx += bdir;
		}
//		bdir = bidx >= lights - 1 ? -1 : bidx == 0 ? 1 : bdir;
		frame += 1;


		for (int j = 0; j < blen; j++) {
			test_neopixel_add_color(lightbuf, (bidx + j) % lights, (0xFF / ((blen - j))) << rdist);
		}
		for (int j = 0; j < glen; j++) {
			test_neopixel_add_color(lightbuf, (gidx + j) % lights, (0xFF / ((glen - j))) << gdist);
		}
		for (int j = 0; j < rlen; j++) {
			test_neopixel_add_color(lightbuf, (ridx + j) % lights, (0xFF / ((j + 1))) << bdist);
		}

		hw_highspeedsignal_update(lightbuf, lightbuf_len);
		hw_wait_ms(10);
	}
}


/**
 * interrupt recording
 */

//	hw_digital_output(E_G6);
//	tm_interrupt_record(2);
//
//	while (1) {
//		tm_sleep_ms(1000);
//		printf("fall\n");
//		hw_digital_write(E_G6, 0);
//		tm_sleep_ms(1000);
//		if (tm_interrupt_record_lastfall(2) - tm_interrupt_record_lastrise(2) > 0) {
//			printf("PULSE WIDTH: %d, %fms\n", 2, ((float) tm_interrupt_record_lastfall(2) - (float) tm_interrupt_record_lastrise(2)) / 1000.0f);
//		}
//		printf("rise\n");
//		hw_digital_write(E_G6, 1);
//	}
