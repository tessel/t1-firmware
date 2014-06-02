// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#ifdef __cplusplus
extern "C" {
#endif


#include "hw.h"
#include "tessel.h"
#include "lpc18xx_uart.h"
#include "lpc18xx_ssp.h"

#define SSPDEV_M LPC_SSP0
#define IDLE_CHAR	0x00

// Write buffer
uint8_t *tm_highspeed_buf = NULL;
// Write index
uint32_t tm_highspeed_buf_idx = 0;
// Data length
uint32_t tm_highspeed_buf_len = 0;

_ramfunc void our_SSP_IntConfig(LPC_SSPn_Type *SSPx, uint32_t IntType, FunctionalState NewState)
{
//	CHECK_PARAM(PARAM_SSPx(SSPx));
//	CHECK_PARAM(PARAM_SSP_INTCFG(IntType));

	if (NewState == ENABLE)
	{
		SSPx->IMSC |= IntType;
	}
	else
	{
		SSPx->IMSC &= (~IntType) & SSP_IMSC_BITMASK;
	}
}

_ramfunc void our_SSP_ClearIntPending(LPC_SSPn_Type *SSPx, uint32_t IntType)
{
//	CHECK_PARAM(PARAM_SSPx(SSPx));
//	CHECK_PARAM(PARAM_SSP_INTCLR(IntType));

	SSPx->ICR = IntType;
}

// _ramfunc void SSP0_IRQHandler(void)
// {
// 	/* check if RX FIFO contains data */
// //	while (SSP_GetStatus(SSPDEV_M, SSP_STAT_RXFIFO_NOTEMPTY) == SET)
// //	{
// //		tmp = SSP_ReceiveData(SSPDEV_M);
// //		if ((pRdBuf_M!= NULL) && (RdIdx_M < DatLen_M))
// //		{
// //			*(pRdBuf_M + RdIdx_M) = (uint8_t) tmp;
// //		}
// //		RdIdx_M++;
// //	}

// 	/* Check if TX FIFO is not full */
// 	while ((SSPDEV_M->SR & SSP_STAT_TXFIFO_NOTFULL))
// 	{
// 		if (tm_highspeed_buf != NULL)
// 		{
// 			SSPDEV_M->DR = SSP_DR_BITMASK(tm_highspeed_buf[tm_highspeed_buf_idx]);
// 		}
// 		else
// 		{
// 			SSPDEV_M->DR = SSP_DR_BITMASK(IDLE_CHAR);
// 		}
// 		if ((++tm_highspeed_buf_idx) >= tm_highspeed_buf_len) {
// 			tm_highspeed_buf_idx = 0;
// 		}
// 	}

// 	our_SSP_IntConfig(SSPDEV_M, SSP_INTCFG_TX, ENABLE);

// //	/* There're more data to send */
// //	if (WrIdx_M < DatLen_M)
// //	{
// //		our_SSP_IntConfig(SSPDEV_M, SSP_INTCFG_TX, ENABLE);
// //	}
// //	/* Otherwise */
// //	else
// //	{
// //		our_SSP_IntConfig(SSPDEV_M, SSP_INTCFG_TX, DISABLE);
// //	}

// 	/* Clear all interrupt */
// 	our_SSP_ClearIntPending(SSPDEV_M, SSP_INTCLR_ROR);
// 	our_SSP_ClearIntPending(SSPDEV_M, SSP_INTCLR_RT);

// //	/* There're more data to receive */
// //	if (RdIdx_M < DatLen_M)
// //	{
// //		SSP_IntConfig(SSPDEV_M, SSP_INTCFG_ROR, ENABLE);
// //		SSP_IntConfig(SSPDEV_M, SSP_INTCFG_RT, ENABLE);
// //		SSP_IntConfig(SSPDEV_M, SSP_INTCFG_RX, ENABLE);
// //	}
// //	/* Otherwise */
// //	else
// //	{
// //		our_SSP_IntConfig(SSPDEV_M, SSP_INTCFG_ROR, DISABLE);
// //		our_SSP_IntConfig(SSPDEV_M, SSP_INTCFG_RT, DISABLE);
// //		our_SSP_IntConfig(SSPDEV_M, SSP_INTCFG_RX, DISABLE);
// //	}

// 	/* Set Flag if both Read and Write completed */
// //	if ((WrIdx_M == DatLen_M) && (RdIdx_M == DatLen_M))
// //	{
// //		complete_M = TRUE;
// //	}
// }

///*-------------------------PRIVATE FUNCTIONS------------------------------*/
///*********************************************************************//**
// * @brief 		SSP Read write in polling mode function (Master mode)
// * @param[in]	SSPx: Pointer to SSP device
// * @param[out]	rbuffer: pointer to read buffer
// * @param[in]	wbuffer: pointer to write buffer
// * @param[in]	length: length of data to read and write
// * @return 		0 if there no data to send, otherwise return 1
// ***********************************************************************/
//int32_t ssp_MasterReadWrite (LPC_SSPn_Type *SSPx,
//	                 void *rbuffer,
//	                 void *wbuffer,
//	                 uint32_t length)
//{
//	pRdBuf_M = (uint8_t *) rbuffer;
//    pWrBuf_M = (uint8_t *) wbuffer;
//    DatLen_M = length;
//    RdIdx_M = 0;
//    WrIdx_M = 0;
//
//    // wait for current SSP activity complete
//    while (SSP_GetStatus(SSPx, SSP_STAT_BUSY));
//
//	/* Clear all remaining data in RX FIFO */
////	while (SSP_GetStatus(SSPx, SSP_STAT_RXFIFO_NOTEMPTY))
////	{
////		SSP_ReceiveData(SSPx);
////	}
//
//////#if (USEDSSPDEV_M == 0)
////	if (length != 0)
////	{
//		SSP0_IRQHandler();
//
//		// write some data
////		SSP_SendData(SSPx, (unt16_t)(*(pWrBuf_M + WrIdx_M++)));
////	}
////#endif
////#if (USEDSSPDEV_M == 1)
////	if (length != 0)
////	{
////		SSP1_IRQHandler();
////	}
////#endif
//	// Return 0
//	return 0;
//}

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

void hw_highspeedsignal_update (uint8_t *buf, size_t buf_len)
{
	uint8_t *tmp = malloc(buf_len);
	memcpy(tmp, buf, buf_len);
	__disable_irq();
	if (tm_highspeed_buf != NULL) {
		free(tm_highspeed_buf);
	}
	tm_highspeed_buf = tmp;
	tm_highspeed_buf_len = buf_len;
	tm_highspeed_buf_idx = 0;
	__enable_irq();

//	for( int i = 0; i < buf_len; i++) {
//		printf(" :: " BYTETOBINARYPATTERN "\n", BYTETOBINARY(tm_highspeed_buf[i]));
//	}

	// SSP0_IRQHandler();
}

int initialized = 0;

void hw_highspeedsignal_initialize (int speed) {
	// TODO fixe
	if (initialized) {
		return;
	}
	initialized = 1;

	hw_spi_initialize(TESSEL_SPI_0, speed, HW_SPI_MASTER, 1, 1, HW_SPI_FRAME_NORMAL);


//	Master_Tx_Buf = calloc(1, 128 + 1 * 9);
//	DatLen_M = 128;

//	SSP0_IRQHandler();
//	ssp_MasterReadWrite(LPC_SSP0, Master_Rx_Buf, Master_Tx_Buf, 0);
//}
	hw_highspeedsignal_update(NULL, 0);

	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(SSP0_IRQn, ((0x01<<3)|0x01));
	/* Enable SSP0 interrupt */
	NVIC_EnableIRQ(SSP0_IRQn);
//	ssp_MasterReadWrite(LPC_SSP0, Master_Rx_Buf, Master_Tx_Buf, buf_len);
//	tm_spi_send(LPC_SSP0, Master_Tx_Buf, BUFFER_SIZE);


	/* Initializing Slave SSP device section -------------------------------------------
	// initialize SSP configuration structure to default
	SSP_ConfigStructInit(&SSP_ConfigStruct);
	// Re-configure mode for SSP device
	SSP_ConfigStruct.Mode = SSP_SLAVE_MODE;
	// Re-configure SSP to TI frame format
	SSP_ConfigStruct.FrameFormat = SSP_FRAME_TI;

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(SSPDEV_S, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(SSPDEV_S, ENABLE);
	*/
	/* Interrupt configuration section -------------------------------------------------
#if ((USEDSSPDEV_S == 0) || (USEDSSPDEV_M == 0))
	// preemption = 1, sub-priority = 1
	NVIC_SetPriority(SSP0_IRQn, ((0x01<<3)|0x01));
	// Enable SSP0 interrupt
	NVIC_EnableIRQ(SSP0_IRQn);
#endif
#if ((USEDSSPDEV_S == 1) || (USEDSSPDEV_M == 1))
	// preemption = 1, sub-priority = 1
	NVIC_SetPriority(SSP1_IRQn, ((0x01<<3)|0x01));
	// Enable SSP0 interrupt
	NVIC_EnableIRQ(SSP1_IRQn);
#endif
	*/
	/* Initializing Buffer section ------------------------------------------------- */
//	Buffer_Init();

	/* Start Transmit/Receive between Master and Slave ----------------------------- */
//	complete_S = FALSE;
//	complete_M = FALSE;

	/* Slave must be ready first */
//	ssp_SlaveReadWrite(SSPDEV_S, Slave_Rx_Buf, Slave_Tx_Buf, BUFFER_SIZE);
	/* Then Master can start its transferring */
}


#ifdef __cplusplus
}
#endif
