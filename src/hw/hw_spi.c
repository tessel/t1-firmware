/*
 * tm_spi.c
 *
 *  Created on: Aug 5, 2013
 *      Author: tim
 */

#include "hw.h"
#include "variant.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_ssp.h"
#include "lpc18xx_gpdma.h"
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
} hw_spi_t;


/**
 * LPC spi list
 */

// TODO: not use A_G1 as the dedicated CS pin
hw_spi_t SPI0 = { LPC_SSP0, 0, MOSI, MISO, 3, 0, MD_PLN, FUNC4, CGU_BASE_SSP0, CGU_PERIPHERAL_SSP0, { 0 } };
hw_spi_t SPI1 = { LPC_SSP1, CC3K_CS, SSP1_MOSI, SSP1_MISO, 1, 19, MD_PUP, ( INBUF_ENABLE | MD_ZI | FUNC1), CGU_BASE_SSP1, CGU_PERIPHERAL_SSP1, { 0 } };

static hw_spi_t* spi_list[] = { &SPI0, &SPI1 };

static hw_spi_t *find_spi (size_t port)
{
	if (port < sizeof(spi_list)) {
		return spi_list[port];
	}
	return spi_list[1]; // DEFAULT SPI1
}

void hw_spi_dma_counter(uint8_t channel){
	// Check counter terminal status
	if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, channel)){
		// Clear terminate counter Interrupt pending
		GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, channel);
		SPI_STATUS.transferCount++;
	}
	if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, channel)){
		// Clear error counter Interrupt pending
		GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, channel);
		SPI_STATUS.transferError++;
	}
}

/**
 * hw_spi_* api
 */

int hw_spi_transfer_async (size_t port, const uint8_t *txbuf, uint8_t *rxbuf, size_t buf_len, size_t* buf_read)
{
	hw_spi_t *SPIx = find_spi(port);

	if (rxbuf != NULL) {
		memset(rxbuf, 0, buf_len);
	}

	// set up TX
	GPDMA_Channel_CFG_Type GPDMACfgTx;

	// channel 0
	GPDMACfgTx.ChannelNum = 0;
	// Source memory
	GPDMACfgTx.SrcMemAddr = (uint32_t)txbuf;
	// Destination memory
	GPDMACfgTx.DstMemAddr = 0;
	// Transfer size
	GPDMACfgTx.TransferSize = buf_len; // number of bytes to transfer
	// Transfer width
	GPDMACfgTx.TransferWidth = GPDMA_WIDTH_BYTE;
	// Transfer type
	GPDMACfgTx.TransferType = GPDMA_TRANSFERTYPE_M2P_CONTROLLER_DMA;
	// Source connection - unused
	GPDMACfgTx.SrcConn = 0;

	if (SPIx->port == LPC_SSP0){
		GPDMACfgTx.DstConn = GPDMA_CONN_SSP0_Tx;
	} else if (SPIx->port == LPC_SSP1){
		GPDMACfgTx.DstConn = GPDMA_CONN_SSP1_Tx;
	}
	
	GPDMACfgTx.DMALLI = 0;

	// set up RX
	GPDMA_Channel_CFG_Type GPDMACfgRx;

	// channel 1
	GPDMACfgRx.ChannelNum = 1;
	// Source memory
	GPDMACfgRx.SrcMemAddr = 0;
	// Destination memory
	GPDMACfgRx.DstMemAddr = (uint32_t)rxbuf;
	// Transfer size
	GPDMACfgRx.TransferSize = buf_len; // number of bytes to transfer
	// Transfer width
	GPDMACfgRx.TransferWidth = GPDMA_WIDTH_BYTE;
	// Transfer type
	GPDMACfgRx.TransferType = GPDMA_TRANSFERTYPE_P2M_CONTROLLER_DMA;

	if (SPIx->port == LPC_SSP0){
		GPDMACfgRx.SrcConn = GPDMA_CONN_SSP0_Rx;
	} else if (SPIx->port == LPC_SSP1){
		GPDMACfgRx.SrcConn = GPDMA_CONN_SSP1_Rx;
	}
	// Destination connection - unused
	GPDMACfgRx.DstConn = 0;
	// Linker List Item - unused
	GPDMACfgRx.DMALLI = 0;
	// // Setup channel with given parameter
	GPDMA_Setup(&GPDMACfgTx);
	GPDMA_Setup(&GPDMACfgRx);
	/* Reset terminal counter */
	SPI_STATUS.transferCount = 0;
	/* Reset Error counter */
	SPI_STATUS.transferError = 0;

	// Enable GPDMA channel 0
	GPDMA_ChannelCmd(0, ENABLE);
	GPDMA_ChannelCmd(1, ENABLE);

	/* Enable GPDMA interrupt */
	NVIC_EnableIRQ(DMA_IRQn);
	
	if (SPI_STATUS.isSlave) {
		// if it's a slave pull down CS
		scu_pinmux(0xF, 1u, PUP_DISABLE | PDN_ENABLE |  MD_EZI | MD_ZI | MD_EHS, FUNC2);  //  SSP0 SSEL0
		
	}

	// hang on this until DMA is done, really this should be pushed back into the event loop
	while ((SPI_STATUS.transferError == 0) && (SPI_STATUS.transferCount == 0));

	// Return the data
	// Caller is responsible for sizing upon return
	if (buf_read != NULL) {
		*buf_read = buf_len;
	}

	return 0;
}


int hw_spi_transfer (size_t port, const uint8_t *txbuf, uint8_t *rxbuf, size_t buf_len, size_t* buf_read)
{
	// sync transfer only supports Master mode
	hw_spi_t *SPIx = find_spi(port);

	SSP_DATA_SETUP_Type xferConfig;

	xferConfig.tx_data = (uint8_t*) txbuf;
	xferConfig.rx_data = rxbuf;
	xferConfig.length = buf_len;

	SSP_ReadWrite(SPIx->port, &xferConfig, SSP_TRANSFER_POLLING);

	// Return the data
	// Caller is responsible for sizing upon return
	if (buf_read != NULL) {
		*buf_read = xferConfig.rx_cnt;
	}

	return 0;
}


int hw_spi_send (size_t port, const uint8_t *txbuf, size_t buf_len)
{
	return hw_spi_transfer(port, txbuf, NULL, buf_len, NULL);
}


int hw_spi_receive (size_t port, uint8_t *rxbuf, size_t buf_len, size_t* buf_read)
{
	return hw_spi_transfer(port, NULL, rxbuf, buf_len, buf_read);
}


int hw_spi_send_async (size_t port, const uint8_t *txbuf, size_t buf_len)
{
	return hw_spi_transfer_async(port, txbuf, NULL, buf_len, NULL);
}


int hw_spi_receive_async (size_t port, uint8_t *rxbuf, size_t buf_len, size_t* buf_read)
{
	return hw_spi_transfer_async(port, NULL, rxbuf, buf_len, buf_read);
}


int hw_spi_disable (size_t port)
{
	hw_spi_t *SPIx = find_spi(port);

	if (SPIx->cs) {
		hw_digital_input(SPIx->cs);
	}

	// Disable the SSP1 peripheral operation
	SSP_Cmd(SPIx->port, DISABLE);

	// Disable it again but seriously this time?
	SSP_DeInit(SPIx->port);

	return 0;
}

static int hw_spi_slave_enable(size_t port){
	hw_spi_t* SPIx = find_spi(port);

	// Enable all lines
	scu_pinmux(3u, 0u,MD_PLN_FAST,FUNC4);  //  SSP0 SCK0
	scu_pinmux(9u, 1u,MD_PLN_FAST,FUNC7);  //  SSP0 MISO0
	scu_pinmux(9u, 2u,MD_PLN_FAST,FUNC7);  //  SSP0 MOSI0
	scu_pinmux(0xF, 1u, PUP_ENABLE | PDN_DISABLE |  MD_EZI | MD_ZI | MD_EHS, FUNC2);  //  SSP0 SSEL0

	SSP_Cmd(SPIx->port, DISABLE);

	// spi slave only supports this one mode for now
	SSP_ConfigStructInit(&SPIx->config);
	SPIx->config.Mode = SSP_SLAVE_MODE;
	SPIx->config.CPHA = SSP_CPHA_SECOND;
	SPIx->config.CPOL = SSP_CPOL_LO;

	SSP_Init(SPIx->port, &SPIx->config);

	// Enable SSP peripheral
	SSP_Cmd(SPIx->port, ENABLE);

	return 0;
}

int hw_spi_enable (size_t port)
{
	hw_spi_t* SPIx = find_spi(port);

	// Enable MOSI/MISO
	hw_digital_output(SPIx->mosi);
	hw_digital_input(SPIx->miso);

	// Start the clock
	scu_pinmux(SPIx->clock_port, SPIx->clock_pin, SPIx->clock_mode, SPIx->clock_func);

	// Configure provided pin as CS
	if (SPIx->cs && g_APinDescription[SPIx->cs].isSlaveSelect == NOTSLAVESELECT) {
		hw_digital_output(SPIx->cs);
		hw_digital_write(SPIx->cs, 1);
	}

	// Enable SSP peripheral
	SSP_Cmd(SPIx->port, ENABLE);

	return 0;
}


int hw_spi_initialize (size_t port, uint32_t clockspeed, uint8_t spimode, uint8_t cpol, uint8_t cpha, uint8_t frameformat)
{
	hw_spi_t* SPIx = find_spi(port);

	if (spimode == HW_SPI_SLAVE) {
		SPI_STATUS.isSlave = 1;
		hw_spi_slave_enable(port);

	} else {
		SPI_STATUS.isSlave = 0;

		// initialize SSP configuration structure to default
		SSP_ConfigStructInit(&SPIx->config);

		// Set the clock speed
		SPIx->config.ClockRate = clockspeed;

		// Set SPOL
		// THIS IS NOT A BUG (yes it is, thanks NXP)
		SPIx->config.CPOL = cpol ? SSP_CPOL_LO : SSP_CPOL_HI;

		// Set CPHA
		SPIx->config.CPHA = cpha ? SSP_CPHA_SECOND : SSP_CPHA_FIRST;

		// Default spi mode is master (which is 0)

		SPIx->config.Mode = SSP_MASTER_MODE;
		SSP_SlaveOutputCmd(SPIx->port, DISABLE);

		// Set the frame format if it was passed in
		SPIx->config.FrameFormat = frameformat;

		// Initialize SSP peripheral with parameters given in structure above
		SSP_Init(SPIx->port, &SPIx->config);

		// Now enable SPI peripheral.
		hw_spi_enable(port);		
	}

	// start up DMA on both lines
	SSP_DMACmd(SPIx->port, SSP_DMA_TX, ENABLE);
	SSP_DMACmd(SPIx->port, SSP_DMA_RX, ENABLE);

	/* Disable GPDMA interrupt */
	NVIC_DisableIRQ(DMA_IRQn);
	/* preemption = 1, sub-priority = 1 */
	NVIC_SetPriority(DMA_IRQn, ((0x01<<3)|0x01));

	// initalize DMA
	GPDMA_Init();
	return 0;
}
