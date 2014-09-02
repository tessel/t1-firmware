// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef _VARIANT_LPC18_X_
#define _VARIANT_LPC18_X_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lpc18xx_scu.h"
#include "lpc18xx_adc.h"

// Pin definitions.

#define ISSLAVESELECT	1
#define NOTSLAVESELECT	0


/**
 * Pin Attributes to be OR-ed
 */
// #define PIN_ATTR_COMBO         (1UL<<0)
// #define PIN_ATTR_ANALOG        (1UL<<1)
// #define PIN_ATTR_DIGITAL       (1UL<<2)
// #define PIN_ATTR_PWM           (1UL<<3)
// #define PIN_ATTR_TIMER         (1UL<<4)


#define NO_ANALOG_CHANNEL -1
#define NO_PORT -1
#define NO_PIN -1
#define NO_FUNC -1
#define NO_BIT -1
#define NO_ALTERNATE -1
#define UART_MODE 1
//#define UART0 LPC_USART0
//#define UART1 LPC_UART1
//#define UART2 LPC_USART2
//#define UART3 LPC_USART3
#define ADC_MODE 2
#define PWM_MODE 3
#define NO_CHANNEL -1

// todo: make this actually useful
#define CTIN_5 5
#define SCT_8 8
#define SCT_5 5
#define SCT_10 10

/* Types used for the tables below */
typedef struct _PinDescription
{
  uint8_t port; // port bank 0...15
  uint8_t pin; // pin number 0...31
  uint8_t mode; // MD_PUP :Pull-up enabled, MD_BUK  :Plain input, MD_PLN  :Repeater mode, MD_PDN  :Pull-down enabled
  uint8_t func; // FUNC0...FUNC7
  uint8_t portNum; // port 0...7
  uint8_t bitNum; // bit number. need to do (1 << (bitNum)) in order to get bitValue
  uint8_t analogChannel;
  uint8_t isSlaveSelect;
  uint8_t alternate; // indicates if it also doubles as I2C, UART, CAN, PWM, etc
  uint8_t alternate_func;
  uint8_t pwm_channel;
  // peripherial should also be here
} PinDescription;

// typedef struct _PinDescription
// {
//   Pio* pPort ;
//   uint32_t ulPin ;
//   uint32_t ulPeripheralId ;
//   EPioType ulPinType ;
//   uint32_t ulPinConfiguration ;
//   uint32_t ulPinAttribute ;
//   EAnalogChannel ulAnalogChannel ; // Analog pin in the Arduino context (label on the board)
//   EAnalogChannel ulADCChannelNumber ; // ADC Channel number in the SAM device
//   EPWMChannel ulPWMChannel ;
//   ETCChannel ulTCChannel ;
// } PinDescription ;

/* Pins table to be instanciated into variant.cpp */
extern const PinDescription *g_APinDescription ;


// Global pinmux arrays.

extern const PinDescription *g_APinDescription;
extern const PinDescription g_APinDescription_boardV0[];
extern const PinDescription g_APinDescription_boardV2[];
extern const PinDescription g_APinDescription_boardV3[];

// PIN LABELS CORRESPOND 1:1 WITH PIN_DESCRIPTION
// EG. g_APinDescription[U1_RXD] SHOULD PROVIDE CORRECT DESCRIPTION OF U1_RXD
// DO NOT CHANGE ORDER OF PINLABELS WITHOUT CHANGING ORDER OF PIN DESCRIPTIONS

enum PinLabel{
	LED1,			//	debug LEDs
	LED2,
	SSP1_CLK,		//	cc3000
	SSP1_MISO,
	SSP1_MOSI,
	CC3K_IRQ,
	CC3K_CS,
	CC3K_SW_EN,
	CC3K_CONN_LED,
	CC3K_ERR_LED,
	CC3K_CONFIG,
	SCL,			//	external
	SDA,
	CLK,
	MISO,
	MOSI,
	A_G1,			//	PORT A GPIO
	A_G2,
	A_G3,
	B_G1,			//	PORT B GPIO
	B_G2,
	B_G3,
	C_G1,			//	PORT C GPIO
	C_G2,
	C_G3,
	D_G1,			//	PORT D GPIO
	D_G2,
	D_G3,
	E_A1,			//	GPIO BANK
	E_A2,
	E_A3,
	E_A4,
	E_A5,
	E_A6,
	E_G1,
	E_G2,
	E_G3,
	E_G4,
	E_G5,
	E_G6,
	BTN1,
	ADC_5,
	ADC_7
};

static inline int hw_valid_pin(int pin) {
	return pin >= 0 && pin <= BTN1;
}

#ifdef __cplusplus
}
#endif

#endif /* _VARIANT_LPC18_X_ */
