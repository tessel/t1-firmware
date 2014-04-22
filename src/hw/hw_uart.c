/*
 * tm_uart.c
 *
 *  Created on: Oct 26, 2013
 *      Author: Jon
 */

#include "hw.h"
#include "tm.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "tessel.h"
#include "colony.h"

/* buffer size definition */
#define UART_RING_BUFSIZE 2048

/* Buf mask */
#define __BUF_MASK (UART_RING_BUFSIZE-1)
/* Check buf is full or not */
#define __BUF_IS_FULL(head, tail) ((tail&__BUF_MASK)==((head+1)&__BUF_MASK))
/* Check buf will be full in next receiving or not */
#define __BUF_WILL_FULL(head, tail) ((tail&__BUF_MASK)==((head+2)&__BUF_MASK))
/* Check buf is empty */
#define __BUF_IS_EMPTY(head, tail) ((head&__BUF_MASK)==(tail&__BUF_MASK))
/* Reset buf */
#define __BUF_RESET(bufidx) (bufidx=0)
#define __BUF_INCR(bufidx)  (bufidx=(bufidx+1)&__BUF_MASK)
/** @brief UART Ring buffer structure */
typedef struct
{
    __IO uint32_t tx_head;                /*!< UART Tx ring buffer head index */
    __IO uint32_t tx_tail;                /*!< UART Tx ring buffer tail index */
    __IO uint32_t rx_head;                /*!< UART Rx ring buffer head index */
    __IO uint32_t rx_tail;                /*!< UART Rx ring buffer tail index */
    __IO uint8_t  *tx;  /*!< UART Tx data ring buffer */
    __IO uint8_t  *rx;  /*!< UART Rx data ring buffer */
} UART_RING_BUFFER_T;

typedef struct UART
{
  tm_event rx_event;
  LPC_USARTn_Type *port;
  UART_RING_BUFFER_T rb;
} UART;

void uart_rx_event_callback(tm_event* event);

UART uarts[3] = {
  {TM_EVENT_INIT(uart_rx_event_callback), LPC_USART0, {0}},
  {TM_EVENT_INIT(uart_rx_event_callback), LPC_USART2, {0}},
  {TM_EVENT_INIT(uart_rx_event_callback), LPC_USART3, {0}},
};

//UART_RING_BUFFER_T rb; // ring buff

// Current Tx Interrupt enable state
__IO FlagStatus TxIntStat;

/************************** PRIVATE FUNCTIONS *************************/
/* Interrupt service routines */
void UART2_IRQHandler(void);
void UART0_IRQHandler(void);
void UART3_IRQHandler(void);
void UART_IntErr(uint8_t bLSErrType);
void UART_IntTransmit(UART* uart);
void UART_IntReceive(UART* uart);
void process_UART_interrupt(UART* uart);

uint32_t hw_uart_rx_available(UART* uart);

/*----------------- INTERRUPT SERVICE ROUTINES --------------------------*/
/*********************************************************************//**
 * @brief   UART2 interrupt handler sub-routine
 * @param[in] None
 * @return    None
 **********************************************************************/
void UART2_IRQHandler(void)
{
  process_UART_interrupt(&uarts[1]);
}

void UART0_IRQHandler(void)
{
  process_UART_interrupt(&uarts[0]);
}

void UART3_IRQHandler(void)
{
  process_UART_interrupt(&uarts[2]);
}

void process_UART_interrupt(UART* uart) {
  uint32_t intsrc, tmp, tmp1;
  LPC_USARTn_Type* UARTPort = uart->port;

  /* Determine the interrupt source */
  intsrc = UARTPort->IIR;
  tmp = intsrc & UART_IIR_INTID_MASK;

  // Receive Line Status
  if (tmp == UART_IIR_INTID_RLS){
    // Check line status
    tmp1 = UARTPort->LSR;
    // Mask out the Receive Ready and Transmit Holding empty status
    tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE \
        | UART_LSR_BI | UART_LSR_RXFE);
    // If any error exist
    if (tmp1) {
      // this keeps saying there is an issue and it's annoying
//      TM_DEBUG("Issue with UART %ul", (unsigned int) tmp1);
//        UART_IntErr(tmp1);
    }
  }

  // Receive Data Available or Character time-out
  if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI)){
      UART_IntReceive(uart);
      tm_event_trigger(&uart->rx_event);
  }

  // Transmit Holding Empty
  if (tmp == UART_IIR_INTID_THRE){
      UART_IntTransmit(uart);
  }
}

/********************************************************************//**
 * @brief     UART receive function (ring buffer used)
 * @param[in] None
 * @return    None
 *********************************************************************/
void UART_IntReceive(UART* uart)
{
  uint8_t tmpc;
  uint32_t rLen;

  LPC_USARTn_Type* UARTPort = uart->port;
  UART_RING_BUFFER_T *rb = &uart->rb;

  while(1){
    // Call UART read function in UART driver
    rLen = UART_Receive(UARTPort, &tmpc, 1, NONE_BLOCKING);
    // If data received
    if (rLen){
      /* Check if buffer is more space
       * If no more space, remaining character will be trimmed out
       */
      if (!__BUF_IS_FULL(rb->rx_head,rb->rx_tail)){
        rb->rx[rb->rx_head] = tmpc;
        __BUF_INCR(rb->rx_head);

      }
    }

    // no more data
    else {
      break;
    }
  }
}

void uart_rx_event_callback(tm_event* event) {
  UART* uart = (UART*) event;
  int portnum = uart - uarts;

  lua_State* L = tm_lua_state;
  
  int bytes_available = hw_uart_rx_available(uart);

  lua_getglobal(L, "_colony_emit");
  lua_pushstring(L, "uart-receive");
  lua_pushnumber(L, portnum);
  uint8_t* buffer = colony_createbuffer(L, bytes_available);
  int bytes_read = hw_uart_receive(portnum, buffer, bytes_available);
  assert(bytes_read == bytes_available);
  tm_checked_call(L, 3);
}

/********************************************************************//**
 * @brief     UART transmit function (ring buffer used)
 * @param[in] None
 * @return    None
 *********************************************************************/
void UART_IntTransmit(UART* uart)
{

  LPC_USARTn_Type* UARTPort = uart->port;
  // Disable THRE interrupt
  UART_IntConfig(UARTPort, UART_INTCFG_THRE, DISABLE);

  /* Wait for FIFO buffer empty, transfer UART_TX_FIFO_SIZE bytes
   * of data or break whenever ring buffers are empty */
  /* Wait until THR empty */
  while (UART_CheckBusy(UARTPort) == SET);

  UART_RING_BUFFER_T *rb = &uart->rb;

  while (!__BUF_IS_EMPTY(rb->tx_head,rb->tx_tail))
    {
        /* Move a piece of data into the transmit FIFO */
      if (UART_Send(UARTPort, (uint8_t *)&rb->tx[rb->tx_tail], 1, NONE_BLOCKING)){
        /* Update transmit ring FIFO tail pointer */
        __BUF_INCR(rb->tx_tail);
      } else {
        break;
      }
    }

    /* If there is no more data to send, disable the transmit
       interrupt - else enable it or keep it enabled */
  if (__BUF_IS_EMPTY(rb->tx_head, rb->tx_tail)) {
      UART_IntConfig(UARTPort, UART_INTCFG_THRE, DISABLE);
      // Reset Tx Interrupt state
      TxIntStat = RESET;
    }
    else{
        // Set Tx Interrupt state
    TxIntStat = SET;
      UART_IntConfig(UARTPort, UART_INTCFG_THRE, ENABLE);
    }
}


/*********************************************************************//**
 * @brief   UART Line Status Error
 * @param[in] bLSErrType  UART Line Status Error Type
 * @return    None
 **********************************************************************/
void UART_IntErr(uint8_t bLSErrType)
{
  (void) bLSErrType;
}

/*-------------------------PRIVATE FUNCTIONS------------------------------*/
/*********************************************************************//**
 * @brief   UART transmit function for interrupt mode (using ring buffers)
 * @param[in] UARTPort  Selected UART peripheral used to send data,
 *        should be UART0
 * @param[out]  txbuf Pointer to Transmit buffer
 * @param[in] buflen Length of Transmit buffer
 * @return    Number of bytes actually sent to the ring buffer
 **********************************************************************/
uint32_t hw_uart_send (uint32_t port, const uint8_t *txbuf, size_t buflen)
{
  LPC_USARTn_Type *UARTPort = uarts[port].port;

    uint8_t *data = (uint8_t *) txbuf;
    uint32_t bytes = 0;

  /* Temporarily lock out UART transmit interrupts during this
     read so the UART transmit interrupt won't cause problems
     with the index values */
  UART_IntConfig(UARTPort, UART_INTCFG_THRE, DISABLE);

  UART_RING_BUFFER_T *rb = &uarts[port].rb;

  /* Loop until transmit run buffer is full or until n_bytes
     expires */
  while ((buflen > 0) && (!__BUF_IS_FULL(rb->tx_head, rb->tx_tail)))
  {
    /* Write data from buffer into ring buffer */
    rb->tx[rb->tx_head] = *data;
    data++;

    /* Increment head pointer */
    __BUF_INCR(rb->tx_head);

    /* Increment data count and decrement buffer size count */
    bytes++;
    buflen--;
  }

  /*
   * Check if current Tx interrupt enable is reset,
   * that means the Tx interrupt must be re-enabled
   * due to call UART_IntTransmit() function to trigger
   * this interrupt type
   */
  if (TxIntStat == RESET) {
    UART_IntTransmit(&uarts[port]);
  }
  /*
   * Otherwise, re-enables Tx Interrupt
   */
  else {
    UART_IntConfig(UARTPort, UART_INTCFG_THRE, ENABLE);
  }

    return bytes;
}


/*********************************************************************//**
 * @brief   UART read function for interrupt mode (using ring buffers)
 * @param[in] UARTPort  Selected UART peripheral used to send data,
 *        should be UART0
 * @param[out]  rxbuf Pointer to Received buffer
 * @param[in] buflen Length of Received buffer
 * @return    Number of bytes actually read from the ring buffer
 **********************************************************************/
uint32_t hw_uart_receive (uint32_t port, uint8_t *rxbuf, size_t buflen)
{
  LPC_USARTn_Type *UARTPort = uarts[port].port;

    uint8_t *data = (uint8_t *) rxbuf;
    uint32_t bytes = 0;

  /* Temporarily lock out UART receive interrupts during this
     read so the UART receive interrupt won't cause problems
     with the index values */
  UART_IntConfig(UARTPort, UART_INTCFG_RBR, DISABLE);

  UART_RING_BUFFER_T *rb = &uarts[port].rb;

  /* Loop until receive buffer ring is empty or
    until max_bytes expires */
  while ((buflen > 0) && (!(__BUF_IS_EMPTY(rb->rx_head, rb->rx_tail))))
  {
    /* Read data from ring buffer into user buffer */
    *data = rb->rx[rb->rx_tail];
    data++;

    /* Update tail pointer */
    __BUF_INCR(rb->rx_tail);

    /* Increment data count and decrement buffer size count */
    bytes++;
    buflen--;
  }

  /* Re-enable UART interrupts */
  UART_IntConfig(UARTPort, UART_INTCFG_RBR, ENABLE);

    return bytes;
}

uint32_t hw_uart_rx_available(UART* uart) {
  return (uart->rb.rx_head - uart->rb.rx_tail)&__BUF_MASK;
}

static void hw_uart_output_tx (uint32_t pin)
{
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    MD_PDN,
    g_APinDescription[pin].alternate_func);
}

static void hw_uart_output_rx (uint32_t pin)
{
  scu_pinmux(g_APinDescription[pin].port,
    g_APinDescription[pin].pin,
    MD_PLN|MD_EZI|MD_ZI,
    g_APinDescription[pin].alternate_func);
}

void hw_uart_enable (uint32_t port)
{
  // Initialize UART pins
  if (port == UART2) { // >TM2 == B
    TM_DEBUG("Enabling UART2 pins");
    if (tessel_board_version() < 2) {
      hw_uart_output_tx(E_G1);
      hw_uart_output_rx(E_G2);
    } else {
      hw_uart_output_tx(B_G1);
      hw_uart_output_rx(B_G2);
    }
  } else if (port == UART0) { // >TM2 == D
    TM_DEBUG("Enabling UART0 pins");
    hw_uart_output_tx(D_G1);
    hw_uart_output_rx(D_G2);
  } else if (port == UART3) { // >TM2 == A
    TM_DEBUG("Enabling UART3 pins");
    hw_uart_output_tx(A_G1);
    hw_uart_output_rx(A_G2);
  }
}

void hw_uart_disable (uint32_t port)
{
  // Disable UART Pins
  if (port == UART2) { // >TM2 == B
    TM_DEBUG("Disabling UART2 pins");
    if (tessel_board_version() < 2) {
      hw_digital_output(E_G1);
      hw_digital_output(E_G2);
    } else {
      hw_digital_output(B_G1);
      hw_digital_output(B_G2);
    }
  } else if (port == UART0) { // >TM2 == D
    TM_DEBUG("Disabling UART0 pins");
    hw_digital_output(D_G1);
    hw_digital_output(D_G2);
  } else if (port == UART3) { // >TM2 == A
    TM_DEBUG("Disabling UART3 pins");
    hw_digital_output(A_G1);
    hw_digital_output(A_G2);
  }
}

void hw_uart_initialize (uint32_t port, uint32_t baudrate, UART_DATABIT_Type databits, UART_PARITY_Type parity, UART_STOPBIT_Type stopbits)
{
  LPC_USARTn_Type *UARTPort = uarts[port].port;

  // UART Configuration structure variable
  UART_CFG_Type UARTConfigStruct;
  // UART FIFO configuration Struct variable
  UART_FIFO_CFG_Type UARTFIFOConfigStruct;

  TM_DEBUG("Setting Baudrate: %i", (int)baudrate);
  TM_DEBUG("Setting databits: %i", databits);
  TM_DEBUG("Setting parity: %i", parity);
  TM_DEBUG("Setting stopbits: %i", stopbits);

  // Set all of our values
  UARTConfigStruct.Baud_rate = baudrate;
  UARTConfigStruct.Databits = databits;
  UARTConfigStruct.Parity = parity;
  UARTConfigStruct.Stopbits = stopbits;

  hw_uart_enable(port);

  // Initialize UART peripheral with given to corresponding parameter
  UART_Init((LPC_USARTn_Type *)UARTPort, &UARTConfigStruct);


  /* Initialize FIFOConfigStruct to default state:
   *        - FIFO_DMAMode = DISABLE
   *        - FIFO_Level = UART_FIFO_TRGLEV0
   *        - FIFO_ResetRxBuf = ENABLE
   *        - FIFO_ResetTxBuf = ENABLE
   *        - FIFO_State = ENABLE
   */
  UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);

  // Initialize FIFO for UART0 peripheral
  UART_FIFOConfig((LPC_USARTn_Type *)UARTPort, &UARTFIFOConfigStruct);


  // Enable UART Transmit
  UART_TxCmd((LPC_USARTn_Type *)UARTPort, ENABLE);

  /* Enable UART Rx interrupt */
  UART_IntConfig((LPC_USARTn_Type *)UARTPort, UART_INTCFG_RBR, ENABLE);
  /* Enable UART line status interrupt */
  UART_IntConfig((LPC_USARTn_Type *)UARTPort, UART_INTCFG_RLS, ENABLE);
  /*
   * Do not enable transmit interrupt here, since it is handled by
   * UART_Send() function, just to reset Tx Interrupt state for the
   * first time
   */
  TxIntStat = RESET;

  UART_RING_BUFFER_T *rb = &uarts[port].rb;

  // Reset ring buf head and tail idx
  __BUF_RESET(rb->rx_head);
  __BUF_RESET(rb->rx_tail);
  __BUF_RESET(rb->tx_head);
  __BUF_RESET(rb->tx_tail);
  if (rb->tx == NULL) {
	  rb->tx = (uint8_t *) malloc(UART_RING_BUFSIZE);
  }
  if (rb->rx == NULL) {
  	  rb->rx = (uint8_t *) malloc(UART_RING_BUFSIZE);
  }

  //TODO: Actually do something with the other ports
  if (port == UART2) {
    /* preemption = 1, sub-priority = 1 */
    NVIC_SetPriority(USART2_IRQn, ((0x01<<3)|0x01));
    /* Enable Interrupt for UART0 channel */
    NVIC_EnableIRQ(USART2_IRQn);
  } else if (port == UART0) {
    /* preemption = 1, sub-priority = 1 */
    NVIC_SetPriority(USART0_IRQn, ((0x01<<3)|0x01));
    /* Enable Interrupt for UART0 channel */
    NVIC_EnableIRQ(USART0_IRQn);
  } else if (port == UART3) {
    /* preemption = 1, sub-priority = 1 */
    NVIC_SetPriority(USART3_IRQn, ((0x01<<3)|0x01));
    /* Enable Interrupt for UART0 channel */
    NVIC_EnableIRQ(USART3_IRQn);
  }

}
