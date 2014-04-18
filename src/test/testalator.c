/*
 * testalator.c
 * c level functions running on the tessel tester
 * tester acts as an i2c slave responding to commands from the tested program
 *
 *  Created on: Jan 18, 2014
 *      Author: jia
 */

#include <stdint.h>
#include <stdlib.h>

#include "hw.h"
#include "tm.h"
#include "tessel.h"
#include "test.h"
#include "variant.h"

// i2c commands
#define PIN_TEST 0x11
#define SCK_TEST 0x21
#define SCK_PORT 0x22
#define SCK_READ 0x23
#define ADC_TEST 0x31
#define DAC_TEST 0x41
#define DAC_READ 0x42
#define I2C_TEST 0x51
#define OK 	0x0F

#define TESTALTOR_PORT 		LPC_I2C1
#define TESTALTOR_ALT_PORT 	LPC_I2C0

#define I2CDEV_S_ADDR 0x2A

uint8_t getCommand(uint8_t *buff, uint8_t len) {
	hw_i2c_slave_receive((uint32_t)TESTALTOR_PORT, buff, len);
	for (int i = 0; i < len; i++){
		TM_DEBUG("received %d", buff[i]);
	}
	return buff[0];
}

void pinTest() {
	hw_digital_write(LED1, 1);
	hw_digital_write(LED2, 0);
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 0);

	// bring all gpios low
	hw_digital_output(A_G1);
	hw_digital_output(A_G2);
	hw_digital_output(A_G3);

	hw_digital_output(B_G1);
	hw_digital_output(B_G2);
	hw_digital_output(B_G3);

	hw_digital_output(C_G1);
	hw_digital_output(C_G2);
	hw_digital_output(C_G3);

	hw_digital_output(D_G1);
	hw_digital_output(D_G2);
	hw_digital_output(D_G3);

	hw_digital_output(E_G1);
	hw_digital_output(E_G2);
	hw_digital_output(E_G3);
	hw_digital_output(E_G4);
	hw_digital_output(E_G5);
	hw_digital_output(E_G6);

	hw_digital_write(A_G1, HW_LOW);
	hw_digital_write(A_G2, HW_LOW);
	hw_digital_write(A_G3, HW_LOW);
	
	hw_digital_write(B_G1, HW_LOW);
	hw_digital_write(B_G2, HW_LOW);
	hw_digital_write(B_G3, HW_LOW);
	
	hw_digital_write(C_G1, HW_LOW);
	hw_digital_write(C_G2, HW_LOW);
	hw_digital_write(C_G3, HW_LOW);

	hw_digital_write(D_G1, HW_LOW);
	hw_digital_write(D_G2, HW_LOW);
	hw_digital_write(D_G3, HW_LOW);

	hw_digital_write(E_G1, HW_LOW);
	hw_digital_write(E_G2, HW_LOW);
	hw_digital_write(E_G3, HW_LOW);
	hw_digital_write(E_G4, HW_LOW);
	hw_digital_write(E_G5, HW_LOW);
	hw_digital_write(E_G6, HW_LOW);

	// respond
	uint8_t txbuff[] = {OK};

	hw_i2c_slave_send((uint32_t)TESTALTOR_PORT, txbuff, 1);
	TM_DEBUG("done sending message");
}

uint8_t sckCount(uint8_t pin) {
	hw_digital_input(pin);
	uint8_t lastVal = 0;
	uint8_t currentVal = 0;
	uint8_t switches = 0;
	// count number of ups and downs
	uint32_t timeout = 10000000;
	while(timeout > 0){
		currentVal = hw_digital_read(pin);
		if (currentVal != lastVal){
			switches++;
		}
		if (switches >= 16) {
			TM_DEBUG("got more than 16 switches");
			return switches;
		}
		timeout--;
		lastVal = currentVal;
	}
	TM_DEBUG("got this many counts %d", switches);
	return switches;
}

void sckTest(uint8_t number) {

	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 1);
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 0);

	// reply with something
	uint8_t ok[1] = {OK};
	hw_i2c_slave_send((uint32_t)TESTALTOR_PORT, ok, 1);
	TM_DEBUG("ok put into sck test mode");

	// wait for 5 port commands
	uint8_t buff[2] = {0x00, 0x00};
	for (int i = 0; i<number; i++){
		TM_DEBUG("sck number %d", i);
		getCommand(buff, 2);
		// uint8_t cmd = buff[0];
		uint8_t port = buff[1];
		uint8_t count = 0;
		switch (port) {
		case 'A':
			// listen on A_G3 pin
			TM_DEBUG("got an A");
			count = sckCount(A_G3);
			break;
		case 'B':
			TM_DEBUG("got a B");
			count = sckCount(B_G3);
			break;
		case 'C':
			TM_DEBUG("got a C");
			count = sckCount(C_G3);
			break;
		case 'D':
			TM_DEBUG("got a D");
			count = sckCount(D_G3);
			break;
		case 'G':
			TM_DEBUG("got a E");
			count = sckCount(E_G3);
			break;
		default:
			TM_DEBUG("got something random %d", port);
			break;
		}

		uint8_t txbuff[2] = {port, count};
		TM_DEBUG("trying to send from i2c slave");
		getCommand(buff, 2);
		hw_i2c_slave_send((uint32_t)TESTALTOR_PORT, txbuff, 2);
	}
}

void adcTest(){
	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 0);
	hw_digital_write(CC3K_ERR_LED, 1);
	hw_digital_write(CC3K_CONN_LED, 0);

	if (tessel_board_version() >= 3){
		hw_analog_write(E_A1, 512);
	} else if (tessel_board_version() <= 2){
		hw_analog_write(E_A4, 512);
	}
	// respond
	uint8_t txbuff[1] = {OK};
	hw_i2c_slave_send((uint32_t)TESTALTOR_PORT, txbuff, 1);
}

void dacTest(){
	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 0);
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 1);

	uint32_t dacReading = 0;
	uint8_t txbuff[5] = {OK, 0x00, 0x00, 0x00, 0x00};

	if (tessel_board_version() >= 3) {
		dacReading = hw_analog_read(E_A1);
	} else if (tessel_board_version() <= 2) {
		dacReading = hw_analog_read(E_A4);
	}
	
	txbuff[1] = dacReading >> 24;
	txbuff[2] = dacReading >> 16;
	txbuff[3] = dacReading >> 8;
	txbuff[4] = dacReading;

	hw_i2c_slave_send((uint32_t)TESTALTOR_PORT, txbuff, 5);
}


void testalator_init(){
	hw_i2c_initialize((uint32_t)TESTALTOR_PORT);
	hw_i2c_enable((uint32_t)TESTALTOR_PORT, I2C_SLAVE);
	hw_i2c_set_slave_addr((uint32_t)TESTALTOR_PORT, I2CDEV_S_ADDR);
	TM_DEBUG("testalator started");
}

void i2cTest(){

	hw_digital_write(LED1, 1);
	hw_digital_write(LED2, 1);
	hw_digital_write(CC3K_ERR_LED, 0);
	hw_digital_write(CC3K_CONN_LED, 0);

	uint8_t txbuff[1] = {OK};
	hw_i2c_slave_send((uint32_t)TESTALTOR_PORT, txbuff, 1);

	// reset txbuff
	txbuff[0] = 0x00;

	// disable regular i2c port
	hw_i2c_disable((uint32_t)TESTALTOR_PORT);

	// enable alt port
	hw_i2c_initialize((uint32_t)TESTALTOR_ALT_PORT);
	hw_i2c_enable((uint32_t)TESTALTOR_ALT_PORT, I2C_SLAVE);
	hw_i2c_set_slave_addr((uint32_t)TESTALTOR_ALT_PORT, I2CDEV_S_ADDR);
	TM_DEBUG("alternate i2c port enabled");

	uint8_t rxbuff[2] = {0x00, 0x00};
	hw_i2c_slave_receive((uint32_t)TESTALTOR_ALT_PORT, rxbuff, 2);
	if (rxbuff[0] == I2C_TEST){
		// sweet it passed
		txbuff[0] = OK;
	}

	hw_i2c_slave_send((uint32_t)TESTALTOR_ALT_PORT, txbuff, 1);
	// re-enable regular port
	hw_i2c_disable((uint32_t)TESTALTOR_ALT_PORT);
	testalator_init();
}


void testalator() {
	testalator_init();

	hw_digital_write(LED1, 1);
	hw_digital_write(LED2, 1);
	hw_digital_write(CC3K_ERR_LED, 1);
	hw_digital_write(CC3K_CONN_LED, 1);

	uint8_t rxbuff[2] = {0x00, 0x00};
	hw_digital_output(B_G1);
  	hw_digital_output(B_G2);

	while (1) {
		rxbuff[0] = 0x00;
		rxbuff[1] = 0x00;
		getCommand(rxbuff, 2);
		switch (rxbuff[0]) {
		case PIN_TEST:
			TM_DEBUG("Testalator: pin test");
			pinTest();
			TM_DEBUG("Testalator: pin done");
			break;
		case SCK_TEST:
			tessel_gpio_init(0);
			TM_DEBUG("Testalator: sck test");
			sckTest(rxbuff[1]);
			TM_DEBUG("Testalator: sck done");
			break;
		case ADC_TEST:
			tessel_gpio_init(0);
			TM_DEBUG("Testalator: adc test");
			adcTest();
			TM_DEBUG("Testalator: adc done");
			break;
		case DAC_TEST:
			tessel_gpio_init(0);
			TM_DEBUG("Testalator: dac test");
			dacTest();
			TM_DEBUG("Testalator: dac done");
			break;
		case I2C_TEST:
			tessel_gpio_init(0);
			TM_DEBUG("Testalator: i2c test");
			i2cTest();
			TM_DEBUG("Testalator: i2c done");
		default:
			break;
		}
	}
}
