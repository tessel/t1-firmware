// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "hw.h"
#include "lpc18xx_gpdma.h"
#include "tm.h"

/**
 * @brief Lookup Table of GPDMA Channel Number matched with
 * GPDMA channel pointer
 */
const LPC_GPDMACH_TypeDef *GPDMA_Channels[8] = {
    LPC_GPDMACH0, // GPDMA Channel 0
    LPC_GPDMACH1, // GPDMA Channel 1
    LPC_GPDMACH2, // GPDMA Channel 2
    LPC_GPDMACH3, // GPDMA Channel 3
    LPC_GPDMACH4, // GPDMA Channel 4
    LPC_GPDMACH5, // GPDMA Channel 5
    LPC_GPDMACH6, // GPDMA Channel 6
    LPC_GPDMACH7, // GPDMA Channel 7
};

volatile const void *GPDMA_Connections[] = {
    NULL,     // SPIFI
    (&LPC_TIMER0->MR),        // MAT0.0
    (&LPC_USART0->THR), // UART0 Tx
    ((uint32_t*)&LPC_TIMER0->MR + 1),       // MAT0.1
    (&LPC_USART0->RBR), // UART0 Rx
    (&LPC_TIMER1->MR),        // MAT1.0
    (&LPC_UART1->THR),  // UART1 Tx
    ((uint32_t*)&LPC_TIMER1->MR + 1),       // MAT1.1
    (&LPC_UART1->RBR),  // UART1 Rx
    (&LPC_TIMER2->MR),        // MAT2.0
    (&LPC_USART2->THR), // UART2 Tx
    ((uint32_t*)&LPC_TIMER2->MR + 1),       // MAT2.1
    (&LPC_USART2->RBR), // UART2 Rx
    (&LPC_TIMER3->MR),        // MAT3.0
    (&LPC_USART3->THR), // UART3 Tx
    NULL,
    ((uint32_t*)&LPC_TIMER3->MR + 1),       // MAT3.1
    (&LPC_USART3->RBR), // UART3 Rx
    NULL,
    (&LPC_SSP0->DR),        // SSP0 Rx
    (&LPC_I2S0->TXFIFO),      // I2S channel 0
    (&LPC_SSP0->DR),        // SSP0 Tx
    (&LPC_I2S0->RXFIFO),      // I2S channel 1
    (&LPC_SSP1->DR),        // SSP1 Rx
    (&LPC_SSP1->DR),        // SSP1 Tx
    (&LPC_ADC0->GDR),       // ADC 0
    (&LPC_ADC1->GDR),       // ADC 1
    (&LPC_DAC->CR)        // DAC
};

/********************************************************************//**
 * @brief   Control which set of peripherals is connected to the
 *        DMA controller
 * @param[in] conn_address  The address of the connection
 * @return  channel number, could be in range: 0..16
 *********************************************************************/
uint8_t TM_DMAMUX_Config(uint32_t connection_number)
{
  uint8_t function, channel;

  switch(connection_number)
  {
    case GPDMA_CONN_SPIFI:    function = 0; channel = 0; break;
    case GPDMA_CONN_MAT0_0:   function = 0; channel = 1; break;
    case GPDMA_CONN_UART0_Tx: function = 1; channel = 1; break;
    case GPDMA_CONN_MAT0_1:   function = 0; channel = 2; break;
    case GPDMA_CONN_UART0_Rx: function = 1; channel = 2; break;
    case GPDMA_CONN_MAT1_0:   function = 0; channel = 3; break;
    case GPDMA_CONN_UART1_Tx: function = 1; channel = 3; break;
    case GPDMA_CONN_MAT1_1:   function = 0; channel = 4; break;
    case GPDMA_CONN_UART1_Rx: function = 1; channel = 4; break;
    case GPDMA_CONN_MAT2_0:   function = 0; channel = 5; break;
    case GPDMA_CONN_UART2_Tx: function = 1; channel = 5; break;
    case GPDMA_CONN_MAT2_1:   function = 0; channel = 6; break;
    case GPDMA_CONN_UART2_Rx: function = 1; channel = 6; break;
    case GPDMA_CONN_MAT3_0:   function = 0; channel = 7; break;
    case GPDMA_CONN_UART3_Tx: function = 1; channel = 7; break;
    case GPDMA_CONN_SCT_0:    function = 2; channel = 7; break;
    case GPDMA_CONN_MAT3_1:   function = 0; channel = 8; break;
    case GPDMA_CONN_UART3_Rx: function = 1; channel = 8; break;
    case GPDMA_CONN_SCT_1:    function = 2; channel = 8; break;
    case GPDMA_CONN_SSP0_Rx:  function = 0; channel = 9; break;
    case GPDMA_CONN_I2S_Channel_0:function = 1; channel = 9; break;
    case GPDMA_CONN_SSP0_Tx:  function = 0; channel = 10; break;
    case GPDMA_CONN_I2S_Channel_1:function = 1; channel = 10; break;
    case GPDMA_CONN_SSP1_Rx:  function = 0; channel = 11; break;
    case GPDMA_CONN_SSP1_Tx:  function = 0; channel = 12; break;
    case GPDMA_CONN_ADC_0:    function = 0; channel = 13; break;
    case GPDMA_CONN_ADC_1:    function = 0; channel = 14; break;
    case GPDMA_CONN_DAC:    function = 0; channel = 15; break;
    default:          function = 3; channel = 15; break;
  }
  //Set select function to dmamux register
  LPC_CREG->DMAMUX &= ~(0x03<<(2*channel));
  LPC_CREG->DMAMUX |= (function<<(2*channel));

  return channel;
}

void* hw_gpdma_get_lli_conn_address(hw_GPDMA_Conn_Type type) {
  return (void*) GPDMA_Connections[type];
}

uint8_t hw_gpdma_transfer_config(uint32_t channelNum, hw_GPDMA_Chan_Config *channelConfig) {
  LPC_GPDMACH_TypeDef *pDMAch;
  uint8_t SrcPeripheral=0, DestPeripheral=0;

  if (LPC_GPDMA->ENBLDCHNS & (GPDMA_DMACEnbldChns_Ch(channelNum))) {
    // This channel is enabled, return ERROR, need to release this channel first
    return ERROR;
  }

  // Get Channel pointer
  pDMAch = (LPC_GPDMACH_TypeDef *) GPDMA_Channels[channelNum];

  // Set correct functions for the peripherals
  SrcPeripheral = TM_DMAMUX_Config(channelConfig->SrcConn);
  DestPeripheral = TM_DMAMUX_Config(channelConfig->DestConn);

  // Reset the Interrupt status
  LPC_GPDMA->INTTCCLEAR = GPDMA_DMACIntTCClear_Ch(channelNum);
  LPC_GPDMA->INTERRCLR = GPDMA_DMACIntErrClr_Ch(channelNum);

  // Clear DMA configure
  pDMAch->CControl = 0x00;
  pDMAch->CConfig = 0x00;

  /* Enable DMA channels, little endian */
  LPC_GPDMA->CONFIG = GPDMA_DMACConfig_E;
  while (!(LPC_GPDMA->CONFIG & GPDMA_DMACConfig_E));

  // Configure DMA Channel, enable Error Counter and Terminate counter
  pDMAch->CConfig = GPDMA_DMACCxConfig_IE | GPDMA_DMACCxConfig_ITC  \
    | GPDMA_DMACCxConfig_TransferType((uint32_t)channelConfig->TransferType) \
    | GPDMA_DMACCxConfig_SrcPeripheral(SrcPeripheral) \
    | GPDMA_DMACCxConfig_DestPeripheral(DestPeripheral);

  TM_DEBUG("Config reg %d", pDMAch->CConfig);

  return SUCCESS;
}

void hw_gpdma_transfer_begin(uint32_t channelNum, hw_GPDMA_Linked_List_Type *firstLLI) {

  // Get the registers for this channel
  LPC_GPDMACH_TypeDef *pDMAch = (LPC_GPDMACH_TypeDef *) GPDMA_Channels[channelNum];

  // Else, it's an address and should be set as such
  pDMAch->CSrcAddr = (uint32_t)firstLLI->Source;

  // Else, it's an address and should be set as such
  pDMAch->CDestAddr = (uint32_t)firstLLI->Destination;
  
  // Set the next linked list item
  pDMAch->CLLI = firstLLI->NextLLI;

  // Set the control registers
  pDMAch->CControl = firstLLI->Control;

  // Enable GPDMA channel
  GPDMA_ChannelCmd(channelNum, ENABLE);

  /* Enable GPDMA interrupt */
  // NVIC_EnableIRQ(DMA_IRQn);
}


/********************************************************************//**
 * @brief     Initialize GPDMA controller
 * @param[in]   None
 * @return    None
 *********************************************************************/
void hw_gpdma_init(void)
{
  GPDMA_Init();
}

void hw_gpdma_cancel_transfer(uint32_t channelNum) {
  GPDMA_ChannelCmd(channelNum, DISABLE);
}