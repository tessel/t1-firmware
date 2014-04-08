#ifndef __TM_I2C__
#define __TM_I2C__

#ifdef __cplusplus
extern "C" {
#endif


// Utility

typedef enum {
	HW_LOW = 0,
	HW_HIGH = 1
} hw_polarity_t;

typedef enum {
	HW_MSB = 0,
	HW_LSB = 1
} hw_sb_t;


// Net

#include "utility/socket.h"
#include "utility/cc3000_common.h"
#include "host_spi.h"

unsigned char hw_net_initialize (void);
void hw_net_config(int should_connect_to_open_ap, int should_use_fast_connect, int auto_start);
void hw_net_smartconfig_initialize(void);
void hw_net_disconnect (void);
// extern uint8_t tm_net_firmware_version ();
int hw_net_connect(const char *security_type, const char *ssid, const char *keys);
int hw_net_connect_wpa2 (const char *ssid, const char *keys);
int hw_net_inuse ();
int hw_net_is_connected (void);
int hw_net_has_ip (void);
void hw_net_block_until_dhcp (void);
int hw_net_block_until_dhcp_wait(int waitLength);

uint32_t hw_net_defaultgateway ();
uint32_t hw_net_dnsserver ();
uint32_t hw_net_dhcpserver ();
uint32_t hw_net_local_ip ();
int hw_net_ssid (char ssid[33]);
void tm_net_initialize_dhcp_server (void);
int hw_net_mac (uint8_t mac[MAC_ADDR_LEN]);
// extern int tm_net_rssi ();

int hw_net_is_readable (int ulSocket);
int hw_net_block_until_readable (int ulSocket, int tries);


// SPI

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

struct spi_status_t{
	uint8_t isSlave;
	uint32_t transferCount;
	uint32_t transferError;
};

volatile struct spi_status_t SPI_STATUS;

int hw_spi_initialize (size_t port, uint32_t clockspeed, uint8_t spimode, uint8_t cpol, uint8_t cpha, uint8_t frameformat);
int hw_spi_enable (size_t port);
int hw_spi_disable (size_t port);
int hw_spi_transfer_async (size_t port, const uint8_t *txbuf, uint8_t *rxbuf, size_t buf_len, size_t* buf_read);
int hw_spi_send_async (size_t port, const uint8_t *txbuf, size_t buf_len);
int hw_spi_receive_async (size_t port, uint8_t *rxbuf, size_t buf_len, size_t* buf_read);
int hw_spi_transfer (size_t port, const uint8_t *txbuf, uint8_t *rxbuf, size_t buf_len, size_t* buf_read);
int hw_spi_send (size_t port, const uint8_t *txbuf, size_t buf_len);
int hw_spi_receive (size_t port, uint8_t *rxbuf, size_t buf_len, size_t* buf_read);
void hw_spi_dma_counter(uint8_t channel);


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

#define UART0 (uint32_t) LPC_USART0
#define UART1 (uint32_t) LPC_UART1
#define UART2 (uint32_t) LPC_USART2
#define UART3 (uint32_t) LPC_USART3
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

#define TM_INTERRUPT_COUNT 8
#define MAX_GPIO_INT 5

enum {
	TM_INTERRUPT_MODE_RISING = 0,
	TM_INTERRUPT_MODE_FALLING = 1
};

extern void initialize_GPIO_interrupts(void);
extern void (* gpio0_callback)(int, int, int);
extern void hw_interrupt_listen (int index, int ulPin, int mode);
void hw_interrupt_callback_attach (int n, void (*callback)(int, int));
void hw_interrupt_callback_detach (int n);

int hw_interrupts_available (void);
int hw_interrupt_watch (int ulPin, int flag, int interruptID);
int hw_interrupt_acquire (void);
int hw_interrupt_assignment_query (int pin);


// highspeed signal

void hw_highspeedsignal_initialize (int speed);
void hw_highspeedsignal_update (uint8_t *buf, size_t buf_len);


// pwm

typedef enum {
  PWM_EDGE_HI = 1,
  PWM_CENTER_HI = 2,
  PWM_EDGE_LOW = 3,
  PWM_CENTER_LOW = 4
} hw_pwm_t;

int hw_pwm_enable (size_t pin, hw_pwm_t mode, uint32_t period, uint32_t pulsewidth);

// gpio

void hw_digital_output (uint8_t ulPin);
void hw_digital_input (uint8_t ulPin);
void hw_digital_startup (uint8_t ulPin);
void hw_digital_write (size_t ulPin, uint8_t ulVal);
uint8_t hw_digital_read (size_t ulPin);

uint32_t hw_analog_read (uint32_t ulPin);
int hw_analog_write (uint32_t ulPin, float ulValue);

#ifdef __cplusplus
};
#endif

#endif
