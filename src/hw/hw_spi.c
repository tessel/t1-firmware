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


volatile struct spi_status_t SPI_STATUS;

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

hw_GPDMA_Linked_List_Type * hw_spi_dma_packetize(size_t buf_len, uint32_t source, uint32_t destination, uint8_t txBool) {
	

	// Get the number of complete packets (0xFFF bytes)
	uint32_t num_full_packets = buf_len/SPI_MAX_DMA_SIZE;
	// Get the number of bytes in final packets (if not a multiple of 0xFFF)
	uint32_t num_remaining_bytes = buf_len % SPI_MAX_DMA_SIZE;
	// Get the total number of packets including incomplete packets
	uint32_t num_linked_lists = num_full_packets + (num_remaining_bytes ? 1 : 0);

	// Generate an array of linked lists to send (will need to be freed)
	hw_GPDMA_Linked_List_Type * Linked_List = calloc(num_linked_lists, sizeof(hw_GPDMA_Linked_List_Type));

	uint32_t base_control;

	// If we are transmitting
	if (txBool) {
		// Use dest AHB Master and source increment
		base_control = 	((1UL<<25)) | ((1UL<<26));
	}
	// If we are receiving
	else {
		// Use source AHB Master and dest increment
		base_control = ((1UL<<24)) | ((1UL<<27));
	}

	// For each packet
	for (uint32_t i = 0; i < num_linked_lists; i++) {
		// Set the source to the same as rx config source
		Linked_List[i].Source = source;
		// Set the detination to where our reading pointer is located
		Linked_List[i].Destination = destination;
		// Se tthe control to read 4 byte bursts with max buf len, and increment the destination counter
		Linked_List[i].Control = base_control;

		// If this is the last packet
		if (i == num_linked_lists-1) {
			// Set this as the end of linked list
			Linked_List[i].NextLLI = (uint32_t)0;
			// Set the interrupt control bit
			Linked_List[i].Control |= ((1UL<<31));
			// If the packet has fewer then max bytes
			if (num_remaining_bytes) {
				// Set those as num to read with interrupt at the end!
				Linked_List[i].Control = num_remaining_bytes | base_control | ((1UL<<31));
			}
		}
		// If it's not the last packet
		else {
			// Set the maximum number of bytes to read
			Linked_List[i].Control |= SPI_MAX_DMA_SIZE;

			// Point next linked list item to subsequent packet
			Linked_List[i].NextLLI = (uint32_t) &Linked_List[i + 1];

			// If this is a transmit buffer
			if (txBool) {
				// Increment source address
				source += SPI_MAX_DMA_SIZE;
			}
			// If this is an receive buffer
			else {
				// Increment the destination address
				destination += SPI_MAX_DMA_SIZE;
			}
		}
	}

	return Linked_List;
}
/**
 * hw_spi_* api
 */

int hw_spi_transfer_async (size_t port, const uint8_t *txbuf, uint8_t *rxbuf, size_t buf_len, size_t* buf_read)
{
	hw_spi_t *SPIx = find_spi(port);

	uint8_t tx_chan = 0;
	uint8_t rx_chan = 1;

	// Clear out the receive buffer
	if (rxbuf != NULL) {
		memset(rxbuf, 0, buf_len);
	}
	
	// set up TX
	hw_GPDMA_Chan_Config tx_config;
	// set up RX
	hw_GPDMA_Chan_Config rx_config;

	// Set the source and destination connection items based on port
	if (SPIx->port == LPC_SSP0){
		tx_config.DestConn = SSP0_TX_CONN;
		rx_config.SrcConn = SSP0_RX_CONN;
	} 
	else {
		tx_config.DestConn = SSP1_TX_CONN;
		rx_config.SrcConn = SSP1_RX_CONN;
	}

	// Source Connection - unused
	tx_config.SrcConn = 0;
	// Transfer type
	tx_config.TransferType = m2p;

	// Destination connection - unused
	rx_config.DestConn = 0;
	// Transfer type
	rx_config.TransferType = p2m;

	// Configure the tx transfer on channel 0
	// TODO: Get next available channel
	hw_gpdma_transfer_config(tx_chan, &tx_config);

	// Configure the rx transfer on channel 1
	// TODO: Get next available channel
	hw_gpdma_transfer_config(rx_chan, &rx_config);


	/* Reset terminal counter */
	SPI_STATUS.transferCount = 0;
	/* Reset Error counter */
	SPI_STATUS.transferError = 0;

	// Generate the linked list structure for transmission
	hw_GPDMA_Linked_List_Type *tx_Linked_List = hw_spi_dma_packetize(buf_len, (uint32_t)txbuf, hw_gpdma_get_lli_conn_address(tx_config.DestConn), 1);

	// Generate the linked list structure for receiving
	hw_GPDMA_Linked_List_Type *rx_Linked_List = hw_spi_dma_packetize(buf_len, hw_gpdma_get_lli_conn_address(rx_config.SrcConn), (uint32_t)rxbuf, 0);

	// Begin the transmission
	hw_gpdma_transfer_begin(tx_chan, tx_Linked_List);
	// Begin the reception
	hw_gpdma_transfer_begin(rx_chan, rx_Linked_List);

	// if it's a slave pull down CS
	if (SPI_STATUS.isSlave) {
		scu_pinmux(0xF, 1u, PUP_DISABLE | PDN_ENABLE |  MD_EZI | MD_ZI | MD_EHS, FUNC2);  //  SSP0 SSEL0
		
	}

	// hang on this until DMA is done, really this should be pushed back into the event loop
	while ((SPI_STATUS.transferError == 0) && (SPI_STATUS.transferCount == 0));

	// Free our linked list (should do this in an event)
	free(tx_Linked_List);
	free(rx_Linked_List);

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
