// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef __TM_I2C__
#define __TM_I2C__

#ifdef __cplusplus
extern "C" {
#endif /** CPP **/

#include <stdint.h>

// Prevent de-definition in CC3000 code.
// data_types.h will check to see #ifndef TRUE, then define
// TRUE as 0. LPC code wants to define TRUE and FALSE as an enum.
// Here we trick the CC3000 into not defining these as constants
// by defining them as their own tokens.
#define FALSE FALSE
#define TRUE TRUE

extern uint8_t hw_wifi_ip[];
extern uint8_t hw_cc_ver[];

// Utility

typedef enum {
	HW_LOW = 0,
	HW_HIGH = 1
} hw_polarity_t;

typedef enum {
	HW_MSB = 0,
	HW_LSB = 1
} hw_sb_t;


// USB

void hw_usb_init(void);

// Net

#include "utility/cc3000_common.h"
#include "utility/data_types.h"
#include "utility/socket.h"
#include "host_spi.h"

void hw_net_initialize (void);
void hw_net_config(int should_connect_to_open_ap, int should_use_fast_connect, int auto_start);
void hw_net_smartconfig_initialize(void);
void hw_net_disable (void);
// extern uint8_t tm_net_firmware_version ();
int hw_net_connect(const char *security_type, const char *ssid, const char *keys);
void hw_net_disconnect (void);
int hw_net_erase_profiles();
int hw_net_inuse ();
int hw_net_is_connected (void);
int hw_net_has_ip (void);
int hw_net_online_status (void);
void hw_net_get_curr_ip(uint8_t * buff);
void hw_net_get_cc_ver(uint8_t * buff);

uint32_t hw_net_defaultgateway ();
uint32_t hw_net_dnsserver ();
uint32_t hw_net_dhcpserver ();
int hw_net_ssid (char ssid[33]);
void tm_net_initialize_dhcp_server (void);
int hw_net_mac (uint8_t mac[MAC_ADDR_LEN]);
// extern int tm_net_rssi ();

int hw_net_is_readable (int ulSocket);
int hw_net_block_until_readable (int ulSocket, int tries);

// general purpose dma
#include "lpc18xx_gpdma.h"

typedef enum {
  SPIFI_CONN = 0,
  MAT0_0_CONN,
  UART0_TX_CONN,
  MAT0_1_CONN,
  UART0_RX_CONN,
  MAT1_0_CONN,
  UART1_TX_CONN,
  MAT1_1_CONN,
  UART1_RX_CONN,
  MAT2_0_CONN,
  UART2_TX_CONN,
  MAT2_1_CONN,
  UART2_RX_CONN,
  MAT3_0_CONN,
  UART3_TX_CONN,
  SCT0_CONN,
  MAT3_1_CONN,
  UART3_RX_CONN,
  SCT1_CONN,
  SSP0_RX_CONN,
  I2S_TX_CONN,
  SSP0_TX_CONN,
  I2S_RX_CONN,
  SSP1_RX_CONN,
  SSP1_TX_CONN,
  ADC0_CONN,
  ADC1_CONN,
  DAC_CONN
} hw_GPDMA_Conn_Type;


typedef enum {
  m2m,
  m2p,
  p2m,
  p2p
} hw_GPDMA_Transfer_Type;

/**
 * @brief GPDMA Channel configuration structure type definition
 */
typedef struct {
  uint32_t SrcConn;   /**< Peripheral Source Connection type
               */
  uint32_t DestConn;   /**< Peripheral Destination Connection type
              */
  hw_GPDMA_Transfer_Type TransferType;
} hw_GPDMA_Chan_Config;

typedef struct {
  uint32_t Source;  /**< Source Address or peripheral */
  uint32_t Destination; /**< Destination address or peripheral */
  uint32_t NextLLI; /**< Next LLI address, otherwise set to '0' */
  uint32_t Control; /**< GPDMA Control of this LLI */
} hw_GPDMA_Linked_List_Type;

void hw_gpdma_init(void);
uint8_t hw_gpdma_transfer_config(uint32_t channelNum, hw_GPDMA_Chan_Config *channelConfig);
void hw_gpdma_transfer_begin(uint32_t channelNum, hw_GPDMA_Linked_List_Type *firstLLI);
void* hw_gpdma_get_lli_conn_address(hw_GPDMA_Conn_Type type);
void hw_gpdma_cancel_transfer(uint32_t channelNum);

// SPI
#include "lpc18xx_ssp.h"

typedef enum {
	HW_SPI_MASTER = 0,
	HW_SPI_SLAVE = 1
} hw_spi_role_t;

typedef enum {
	HW_SPI_FRAME_NORMAL = 0,
	HW_SPI_FRAME_TI = 1,
	HW_SPI_FRAME_MICROWIRE = 2
} hw_spi_frame_t;

typedef enum {
	HW_SPI_LOW = 0,
	HW_SPI_HIGH = 1
} hw_spi_polarity_t;

typedef enum {
	HW_SPI_FIRST = 0,
	HW_SPI_SECOND = 1
} hw_spi_phase_t;

struct spi_async_status_t {
  uint32_t transferCount;
  uint32_t transferError;
  hw_GPDMA_Linked_List_Type *tx_Linked_List;
  hw_GPDMA_Linked_List_Type *rx_Linked_List;
  uint32_t txLength;
  uint32_t rxLength;
  int32_t txRef;
  int32_t rxRef;
  const uint8_t * txbuf;
  uint8_t * rxbuf;
  hw_GPDMA_Chan_Config tx_config;
  hw_GPDMA_Chan_Config rx_config;
  // Callback is a shim to allow C code to receive callbacks
  void (*callback)();
};

// Configuration for a port
typedef struct hw_spi {
  LPC_SSPn_Type *port;
  int cs;
  int mosi;
  int miso;
  int clock_port;
  int clock_pin;
  int clock_mode;
  int clock_func;
  int cgu_base;
  int cgu_peripheral;
  SSP_CFG_Type config;
  int is_slave;
} hw_spi_t;

#define SPI_MAX_DMA_SIZE 0xFFF

extern struct spi_async_status_t spi_async_status;

hw_spi_t * find_spi (size_t port);
int hw_spi_initialize (size_t port, uint32_t clockspeed, uint8_t spimode, uint8_t cpol, uint8_t cpha, uint8_t frameformat);
int hw_spi_enable (size_t port);
int hw_spi_disable (size_t port);
void _hw_spi_irq_interrupt();


int hw_spi_transfer (size_t port, size_t txlen, size_t rxlen, const uint8_t *txbuf, uint8_t *rxbuf, uint32_t txref, uint32_t rxref, void (*callback)());
void hw_spi_dma_counter (uint8_t channel);
void hw_spi_async_cleanup (void);

int hw_spi_transfer_sync (size_t port, const uint8_t *txbuf, uint8_t *rxbuf, size_t buf_len, size_t* buf_read);
int hw_spi_send_sync (size_t port, const uint8_t *txbuf, size_t buf_len);
int hw_spi_receive_sync (size_t port, uint8_t *rxbuf, size_t buf_len, size_t* buf_read);



// I2C

#include <stddef.h>
#include <stdint.h>

#include "lpc18xx_i2c.h"
#define TM_I2C_1 (uint32_t) LPC_I2C1
#define TM_I2C_0 (uint32_t) LPC_I2C0
#define I2C_MASTER  I2C_MASTER_MODE
#define I2C_SLAVE I2C_SLAVE_MODE
#define I2C_GENERAL I2C_GENERAL_MODE,

void hw_i2c_initialize (uint32_t port);
void hw_i2c_enable (uint32_t port, uint8_t mode);
void hw_i2c_disable (uint32_t port);
int hw_i2c_master_transfer (uint32_t port, uint32_t addr, const uint8_t *txbuf, size_t txbuf_len, uint8_t *rxbuf, size_t rxbuf_len);
int hw_i2c_master_send (uint32_t port, uint32_t addr, const uint8_t *txbuf, size_t txbuf_len);
int hw_i2c_master_receive (uint32_t port, uint32_t addr, uint8_t *rxbuf, size_t rxbuf_len);
int hw_i2c_slave_transfer (uint32_t port, const uint8_t *txbuf, size_t txbuf_len, uint8_t *rxbuf, size_t rxbuf_len);
int hw_i2c_slave_send (uint32_t port, const uint8_t *txbuf, size_t txbuf_len);
int hw_i2c_slave_receive (uint32_t port, uint8_t *rxbuf, size_t rxbuf_len);
void hw_i2c_set_slave_addr (uint32_t port, uint8_t slave_addr);

// uart

#include "lpc18xx_uart.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_libcfg.h"
#include "lpc18xx_scu.h"

#define UART0 0
#define UART2 1
#define UART3 2
#define UART_SW_0 0
#define UART_SW_1 1

void hw_uart_enable(uint32_t port);
void hw_uart_disable(uint32_t port);
void hw_uart_initialize(uint32_t UARTPort, uint32_t baudrate, UART_DATABIT_Type databits, UART_PARITY_Type parity, UART_STOPBIT_Type stopbits);
uint32_t hw_uart_receive(uint32_t UARTPort, uint8_t *rxbuf, size_t buflen);
uint32_t hw_uart_send(uint32_t UARTPort, const uint8_t *txbuf, size_t buflen);


// software uart

#include "lpc18xx_uart.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_libcfg.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_timer.h"

void sw_uart_test_c(void);


// usb

#include <stdint.h>
#include "lpc_types.h"
#include <string.h>

int hw_send_usb_msg(unsigned tag, const unsigned char* msg, unsigned length);
int hw_send_usb_msg_formatted(unsigned tag, const char* format, ...);
#define TM_COMMAND(command, str, ...) hw_send_usb_msg_formatted(command, str, ##__VA_ARGS__)


// wait

void hw_wait_ms (int ms);
void hw_wait_us (int us);


// interrupts

// Available for use in JS
#define NUM_INTERRUPTS 7

// ID reserved for CC3K
#define CC3K_GPIO_INTERRUPT 7

typedef enum {
	TM_INTERRUPT_MODE_RISING = 0,
	TM_INTERRUPT_MODE_FALLING = 1,
	TM_INTERRUPT_MODE_HIGH = 2,
	TM_INTERRUPT_MODE_LOW = 3,
	TM_INTERRUPT_MODE_CHANGE = 4
} InterruptMode;

#define TM_INTERRUPT_MASK_BIT_RISING 1 << TM_INTERRUPT_MODE_RISING
#define TM_INTERRUPT_MASK_BIT_FALLING 1 << TM_INTERRUPT_MODE_FALLING
#define TM_INTERRUPT_MASK_BIT_HIGH 1 << TM_INTERRUPT_MODE_HIGH
#define TM_INTERRUPT_MASK_BIT_LOW 1 << TM_INTERRUPT_MODE_LOW

extern void initialize_GPIO_interrupts(void);

// Low-level pin interrupt setup
void hw_interrupt_enable(int index, int ulPin, int bitMask);
void hw_interrupt_disable(int index, int bitMask);

int hw_interrupts_available (void);
int hw_interrupt_watch (int ulPin, int bitMask, int interruptID, void (*callback)());
int hw_interrupt_unwatch(int interrupt_index, int bitMask);
int hw_interrupt_acquire (void);
int hw_interrupt_assignment_query (int pin);


// highspeed signal

void hw_highspeedsignal_initialize (int speed);
void hw_highspeedsignal_update (uint8_t *buf, size_t buf_len);


// pwm

int hw_pwm_port_period (uint32_t period);
int hw_pwm_pin_pulsewidth (int pin, uint32_t pulsewidth);

// gpio

void hw_digital_output (uint8_t ulPin);
void hw_digital_input (uint8_t ulPin);
void hw_digital_startup (uint8_t ulPin);
void hw_digital_write (size_t ulPin, uint8_t ulVal);
uint8_t hw_digital_read (size_t ulPin);
uint8_t hw_digital_get_mode (uint8_t ulPin);
int hw_digital_set_mode (uint8_t ulPin, uint8_t mode);

uint32_t hw_analog_read (uint32_t ulPin);
int hw_analog_write (uint32_t ulPin, float ulValue);

#ifdef __cplusplus
};
#endif /**CPP**/

#endif /**__TM_I2C__**/
