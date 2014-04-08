#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tessel.h"
#include "hw.h"
#include "host_spi.h"
#include "utility/wlan.h"
#include "utility/nvmem.h"
#include "utility/security.h"
#include "utility/hci.h"
//#include "utility/os.h"
#include "utility/evnt_handler.h"
#include "lpc18xx_timer.h"

#include "lpc18xx_ssp.h"


void set_cc3k_irq_flag (uint8_t val);
uint8_t get_cc3k_irq_flag ();

void CC_BLOCKS () {
	// bad CC! you probably are waiting for a SPI call.
	if (get_cc3k_irq_flag()) {
		hw_digital_write(CC3K_ERR_LED, 0);
		set_cc3k_irq_flag(0);
		SPI_IRQ();
	}
}


#define READ                    3
#define WRITE                   1

#define HI(value)               (((value) & 0xFF00) >> 8)
#define LO(value)               ((value) & 0x00FF)

// #define ASSERT_CS()          (P1OUT &= ~BIT3)

// #define DEASSERT_CS()        (P1OUT |= BIT3)

#define HEADERS_SIZE_EVNT       (SPI_HEADER_SIZE + 5)

#define SPI_HEADER_SIZE			(5)

#define 	eSPI_STATE_POWERUP 				 (0)
#define 	eSPI_STATE_INITIALIZED  		 (1)
#define 	eSPI_STATE_IDLE					 (2)
#define 	eSPI_STATE_WRITE_IRQ	   		 (3)
#define 	eSPI_STATE_WRITE_FIRST_PORTION   (4)
#define 	eSPI_STATE_WRITE_EOT			 (5)
#define 	eSPI_STATE_READ_IRQ				 (6)
#define 	eSPI_STATE_READ_FIRST_PORTION	 (7)
#define 	eSPI_STATE_READ_EOT				 (8)

#if 0
	#define CC3000_nIRQ   (GPIO0_12) //GPIO0_7
	#define HOST_nCS    (GPIO0_2)//(SSP1_SSEL)
	#define HOST_VBAT_SW_EN (GPIO0_13) //GPIO5_2
	#define LED 			(LED3)
#elseif 0
	#define CC3000_nIRQ 	(GPIO0_7)
	#define HOST_nCS		(GPIO1_1)//(SSP1_SSEL)
	#define HOST_VBAT_SW_EN (GPIO5_2)
	#define LED 			(LED3)
#else
	#define CC3000_nIRQ 	(CC3K_IRQ)
	#define HOST_nCS		(CC3K_CS)
	#define HOST_VBAT_SW_EN (CC3K_SW_EN)
	#define LED 			(CC3K_CONN_LED)
#endif

#define DISABLE										(0)

#define ENABLE										(1)

#define NETAPP_IPCONFIG_MAC_OFFSET				(20)
#define DEBUG_LED (1)

#ifdef CC3000_DEBUG
	#define DEBUG_MODE 1
	#include "tm.h"
#else
	#define DEBUG_MODE 0
	#define TM_DEBUG(...)
#endif

// TODO remove this line, fix these definitions
//#define delayMicroseconds(x)			tm_sleep_us(x)


unsigned char tSpiReadHeader[] = {READ, 0, 0, 0, 0};

//foor spi bus loop
int loc = 0; 

char ssid[] = "";                     // your network SSID (name) 
unsigned char keys[] = "";       // your network key
// c4:10:8a:57:8e:68
unsigned char bssid[] = {0xc4, 0x10, 0x8a, 0x57, 0x8c, 0x18};       // your network key
char device_name[] = "bobby";
static const char aucCC3000_prefix[] = {'T', 'T', 'T'};
static const unsigned char smartconfigkey[] = {0x73,0x6d,0x61,0x72,0x74,0x63,0x6f,0x6e,0x66,0x69,0x67,0x41,0x45,0x53,0x31,0x36};

//00:11:95:41:38:65
// unsigned char bssid[] = "000000";
int keyIndex = 0; 
unsigned char printOnce = 1;

unsigned volatile long ulSmartConfigFinished, ulCC3000Connected, ulCC3000DHCP, OkToDoShutDown, ulCC3000DHCP_configured, smartconfig_process;

unsigned char ucStopSmartConfig;

typedef struct
{
	gcSpiHandleRx  SPIRxHandler;
	unsigned short usTxPacketLength;
	unsigned short usRxPacketLength;
	volatile unsigned long  ulSpiState;
	unsigned char *pTxPacket;
	unsigned char *pRxPacket;

} tSpiInformation;


tSpiInformation sSpiInformation;

void SPI_IRQCallback(int, int);
void SpiWriteDataSynchronous(unsigned char *data, unsigned short size);
void SpiWriteAsync(const unsigned char *data, unsigned short size);
void SpiPauseSpi(void);
void SpiResumeSpi(void);
void SSIContReadOperation(void);
void SpiReadHeader(void);


// The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
// for the purpose of detection of the overrun. The location of the memory where the magic number 
// resides shall never be written. In case it is written - the overrun occured and either recevie function
// or send function will stuck forever.
#define CC3000_BUFFER_MAGIC_NUMBER (0xDE)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//#pragma is used for determine the memory location for a specific variable.                            ///        ///
//__no_init is used to prevent the buffer initialization in order to prevent hardware WDT expiration    ///
// before entering to 'main()'.                                                                         ///
//for every IDE, different syntax exists :          1.   __CCS__ for CCS v5                    ///
//                                                  2.  __IAR_SYSTEMS_ICC__ for IAR Embedded Workbench  ///
// *CCS does not initialize variables - therefore, __no_init is not needed.                             ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned char wlan_tx_buffer[CC3000_TX_BUFFER_SIZE];
unsigned char spi_buffer[CC3000_RX_BUFFER_SIZE];

void csn(uint8_t mode)
{
	hw_digital_write(HOST_nCS, mode);
}

//*****************************************************************************
// 
//!  This function get the reason for the GPIO interrupt and clear cooresponding
//!  interrupt flag
//! 
//!  \param  none
//! 
//!  \return none
//! 
//!  \brief  This function This function get the reason for the GPIO interrupt
//!          and clear cooresponding interrupt flag
// 
//*****************************************************************************
void SpiCleanGPIOISR(void)
{
	if (DEBUG_MODE)
	{
		TM_DEBUG("SpiCleanGPIOISR");
	}

	//add code
}
 


//*****************************************************************************
//
//!  SpiClose
//!
//!  \param  none
//!
//!  \return none
//!
//!  \brief  Cofigure the SSI
//
//*****************************************************************************
void
SpiClose(void)
{
	if (DEBUG_MODE)
	{
//		TM_DEBUG("SpiClose");
	}

	if (sSpiInformation.pRxPacket)
	{
		sSpiInformation.pRxPacket = 0;
	}

	// //
	// //	Disable Interrupt in GPIOA module...
	// //
	tSLInformation.WlanInterruptDisable();
}


void SpiInit(uint32_t clock_speed) {
	hw_digital_output(HOST_nCS);
	hw_digital_output(HOST_VBAT_SW_EN);
	//Initialize SPI
	hw_spi_initialize(TESSEL_SPI_1, clock_speed, HW_SPI_MASTER, 0, 1, HW_SPI_FRAME_NORMAL);
	
	csn(HW_HIGH);
}

//*****************************************************************************
//
//!  SpiClose
//!
//!  \param  none
//!
//!  \return none
//!
//!  \brief  Cofigure the SSI
//
//*****************************************************************************
void SpiOpen(gcSpiHandleRx pfRxHandler)
{

	if (DEBUG_MODE)
	{
//		TM_DEBUG("SpiOpen");
	}

	sSpiInformation.ulSpiState = eSPI_STATE_POWERUP;

	sSpiInformation.SPIRxHandler = pfRxHandler;
	sSpiInformation.usTxPacketLength = 0;
	sSpiInformation.pTxPacket = NULL;
	sSpiInformation.pRxPacket = (unsigned char *)spi_buffer;
	sSpiInformation.usRxPacketLength = 0;
	spi_buffer[CC3000_RX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;
	wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1] = CC3000_BUFFER_MAGIC_NUMBER;


	 
//	Enable interrupt on the GPIOA pin of WLAN IRQ
	
	tSLInformation.WlanInterruptEnable();


	if (DEBUG_MODE)
	{
//		TM_DEBUG("Completed SpiOpen");

	}
}




//*****************************************************************************
//
//! This function: init_spi
//!
//!  \param  buffer
//!
//!  \return none
//!
//!  \brief  initializes an SPI interface
//
//*****************************************************************************

void fakeWait(long waitLoop){
	while(waitLoop-- > 0)
	{

	}
	return;
}


long SpiFirstWrite(unsigned char *ucBuf, unsigned short usLength)
{
 
	//
    // workaround for first transaction
    //
	if (DEBUG_MODE)
	{
//		TM_DEBUG("SpiFirstWrite");
	}

  // hw_digital_write(HOST_nCS, 0);
	csn(HW_LOW);
	fakeWait(1100);
	
	// SPI writes first 4 bytes of data
	SpiWriteDataSynchronous(ucBuf, 4);

	fakeWait(880);
	while(SSP_GetStatus(LPC_SSP1, SSP_STAT_BUSY)){
		CC_BLOCKS();
	};
//	 unsigned char testData[6];
//	 testData[0] = (unsigned char)0x00;
//	 testData[1] = (unsigned char)0x01;
//	 testData[2] = (unsigned char)0x00;
//	 testData[3] = (unsigned char)0x40;
//	 testData[4] = (unsigned char)0x0E;
//	 testData[5] = (unsigned char)0x04;

	SpiWriteDataSynchronous(ucBuf + 4, usLength - 4);
//	 SpiWriteDataSynchronous(testData, usLength - 4);
	//while(SSP_GetStatus(LPC_SSP1, SSP_STAT_BUSY)){};

	// From this point on - operate in a regular way
	sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
//	fakeWait(1100);
//	fakeWait(450);
//	fakeWait(10);
//	fakeWait(20);



//	 hw_digital_write(HOST_nCS, 1);
	csn(HW_HIGH);
	return(0);
}


long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength)
{
//	 if (DEBUG_MODE)
//	 {
//	 	TM_DEBUG("SpiWrite");
//	 }

	unsigned char ucPad = 0;

	//
	// Figure out the total length of the packet in order to figure out if there is padding or not
	//
	if(!(usLength & 0x0001))
	{
		ucPad++;
	}
	
	pUserBuffer[0] = WRITE;
	pUserBuffer[1] = HI(usLength + ucPad);
	pUserBuffer[2] = LO(usLength + ucPad);
	pUserBuffer[3] = 0;
	pUserBuffer[4] = 0;

	usLength += (SPI_HEADER_SIZE + ucPad);

	// The magic number that resides at the end of the TX/RX buffer (1 byte after the allocated size)
	// for the purpose of detection of the overrun. If the magic number is overriten - buffer overrun 
	// occurred - and we will stuck here forever!
	uint32_t buffer_check = wlan_tx_buffer[CC3000_TX_BUFFER_SIZE - 1];
	if (buffer_check != CC3000_BUFFER_MAGIC_NUMBER)
	{
		while (1)
			;
	}

	// TM_DEBUG("Checking for state");
	// print_spi_state();
	if (sSpiInformation.ulSpiState == eSPI_STATE_POWERUP)
	{
//		TM_DEBUG("Waiting for SpiState to become initialized...");
//		TM_DEBUG("waiting for eSPI_STATE_INITIALIZED");
		while (sSpiInformation.ulSpiState != eSPI_STATE_INITIALIZED){
			CC_BLOCKS();
		}
			;
//		TM_DEBUG("done waiting for eSPI_STATE_INITIALIZED");
	}

	if (sSpiInformation.ulSpiState == eSPI_STATE_INITIALIZED)
	{
//		TM_DEBUG("SpiState is initialized");
		//
		// This is time for first TX/RX transactions over SPI: the IRQ is down - so need to send read buffer size command
		//
		SpiFirstWrite(pUserBuffer, usLength);
	}
	else 
	{
		
		// We need to prevent here race that can occur in case 2 back to back 
		// packets are sent to the  device, so the state will move to IDLE and once 
		//again to not IDLE due to IRQ
		tSLInformation.WlanInterruptDisable();
//		TM_DEBUG("waiting for eSPI_STATE_IDLE");
		while (sSpiInformation.ulSpiState != eSPI_STATE_IDLE)
		{
			CC_BLOCKS();
		}
//		TM_DEBUG("done waiting for eSPI_STATE_IDLE");
		sSpiInformation.ulSpiState = eSPI_STATE_WRITE_IRQ;
		sSpiInformation.pTxPacket = pUserBuffer;
		sSpiInformation.usTxPacketLength = usLength;

		// reenable IRQ
		tSLInformation.WlanInterruptEnable();
		// assert CS
		csn(HW_LOW);
//		TM_DEBUG("dropped CS");

	}

//	if (tSLInformation.ReadWlanInterruptPin() == 0) {
//
//		// check for a missing interrupt between the CS assertion and enabling back the interrupts
//		if (sSpiInformation.ulSpiState == eSPI_STATE_WRITE_IRQ)
//		{
//			haswritten = 0;
//			// TM_DEBUG("writing synchronous data");
//			SpiWriteDataSynchronous(sSpiInformation.pTxPacket, sSpiInformation.usTxPacketLength);
//			sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
//
//			//deassert CS
//			//hw_digital_write(HOST_nCS, 1);
//			csn(HW_HIGH);
//		}
//	}
	
	//
	// Due to the fact that we are currently implementing a blocking situation
	// here we will wait till end of transaction
	//
//	TM_DEBUG("waiting for sSpiInformation.ulSpiState");
	while (eSPI_STATE_IDLE != sSpiInformation.ulSpiState) {
		CC_BLOCKS();
		continue;
	}
//	TM_DEBUG("done waiting for sSpiInformation.ulSpiState");
//	if (haswritten > 0) {
////		TM_DEBUG("pulling CSN high");
//		csn(HW_HIGH); // ALLOW IRQ to trigger again
//	}
//	TM_DEBUG("done with host_spi write");
	// TM_DEBUG("done with spi write");
    return(0);

}

void renableIRQ(void) {
	csn(HW_HIGH); // ALLOW IRQ to trigger again
}

void SpiWriteDataSynchronous(unsigned char *data, unsigned short size)
{
	tSLInformation.WlanInterruptDisable();
//	if (DEBUG_MODE) {
//		TM_DEBUG("writing data");
//	}
//	 if (DEBUG_MODE)
//	 {
//	 	TM_DEBUG("SpiWriteDataSynchronous");
//	 	for(int i = 0; i<size; i++) {
//	 		TM_DEBUG("data[%d]: %d", i, data[i]);
//	 	}
//	 	TM_DEBUG("");
//	 }
	
	// csn(HW_LOW);
	while (size--) {
		hw_spi_send(TESSEL_SPI_1, data, 1);
		data+=1;
	}
	// csn(HW_HIGH);
	
	// if (DEBUG_MODE)
	// {
	// 	TM_DEBUG("SpiWriteDataSynchronous done.");
	// 	delayMicroseconds(50);
	// }
	tSLInformation.WlanInterruptEnable();
	
}


void SpiReadDataSynchronous(unsigned char *data, unsigned short size)
{
	unsigned int i = 0;
	for (i = 0; i < size; i ++) {
		hw_spi_transfer(TESSEL_SPI_1, &(tSpiReadHeader[0]), &(data[i]), 1, NULL);
	}
}

void SpiReadHeader(void)
{

	// if (DEBUG_MODE)
	// {
	// 	TM_DEBUG("SpiReadHeader");
	// }

	//SpiWriteDataSynchronous(tSpiReadHeader, 3);
//	TM_DEBUG("reading header\n");
	SpiReadDataSynchronous(sSpiInformation.pRxPacket, 10);

//	TM_DEBUG("here is the header buff");
//	for(int test = 0; test<10; test++){
//		TM_DEBUG("header received %ul", ((unsigned char *)(sSpiInformation.pRxPacket))[test]);
//	}
}



long SpiReadDataCont(void)
{

	// if (DEBUG_MODE)
	// {
	// 	TM_DEBUG("SpiReadDataCont");
	// }
	long data_to_recv;
	unsigned char *evnt_buff, type;

	
	//
	//determine what type of packet we have
	//
	evnt_buff =  sSpiInformation.pRxPacket;
	data_to_recv = 0;
	STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), 
		HCI_PACKET_TYPE_OFFSET, type);
	
  switch(type)
  {
    case HCI_TYPE_DATA:
    {
			//
			// We need to read the rest of data..
			//
//    		TM_DEBUG("data data_to_recv %ul", data_to_recv);
			STREAM_TO_UINT16((char *)(evnt_buff + SPI_HEADER_SIZE), 
				HCI_DATA_LENGTH_OFFSET, data_to_recv);
			if (!((HEADERS_SIZE_EVNT + data_to_recv) & 1))
			{
				data_to_recv++;
			}

			if (data_to_recv)
			{
				SpiReadDataSynchronous(evnt_buff + 10, data_to_recv);
			}
//			TM_DEBUG("here is data buff");
//			for(int test = 0; test<data_to_recv; test++){
//				TM_DEBUG("data received %ul", ((unsigned char *)(evnt_buff))[10+test]);
//			}
			break;
		}
		case HCI_TYPE_EVNT:
		{
		// 
		// Calculate the rest length of the data
		//

			STREAM_TO_UINT8((char *)(evnt_buff + SPI_HEADER_SIZE), 
				HCI_EVENT_LENGTH_OFFSET, data_to_recv);
			data_to_recv -= 1;
			
//			TM_DEBUG("event data_to_recv %ul", data_to_recv);
			// 
			// Add padding byte if needed
			//
			if ((HEADERS_SIZE_EVNT + data_to_recv) & 1)
			{
				data_to_recv++;
			}
			
			if (data_to_recv)
			{
				SpiReadDataSynchronous(evnt_buff + 10, data_to_recv);
			}
//			TM_DEBUG("received %ul", data_to_recv);
			// read through event buff
//			TM_DEBUG("here is event buff");
//			for(int test = 0; test<data_to_recv; test++){
//				TM_DEBUG("event received %ul", ((unsigned char *)(evnt_buff))[10+test]);
//			}
			sSpiInformation.ulSpiState = eSPI_STATE_READ_EOT;
			break;
		}
	}
	
	return (0);
}

void SpiPauseSpi(void)
{
	// if (DEBUG_MODE)
	// {
	// 	TM_DEBUG("SpiPauseSpi");
	// }

	set_cc3k_irq_flag(0);
	hw_interrupt_callback_detach(0);	//Detaches Pin 3 from interrupt 1
}

void SpiResumeSpi(void)
{
	// if (DEBUG_MODE)
	// {
	// 	TM_DEBUG("SpiResumeSpi");
	// }

	hw_interrupt_callback_attach(0, SPI_IRQCallback); //Attaches Pin 2 to interrupt 1
	if (get_cc3k_irq_flag()) {
		SPI_IRQ();
	}
}

void SpiTriggerRxProcessing(void)
{

	
	// //
	// // Trigger Rx processing
	// //
	SpiPauseSpi();
	csn(HW_HIGH);
	//DEASSERT_CS();
	//hw_digital_write(HOST_nCS, 1);


	// The magic number that resides at the end of the TX/RX buffer (1 byte after 
	// the allocated size) for the purpose of detection of the overrun. If the 
	// magic number is overwritten - buffer overrun occurred - and we will stuck 
	// here forever!
	uint32_t buffer_check = spi_buffer[CC3000_RX_BUFFER_SIZE - 1];
	if ( buffer_check != CC3000_BUFFER_MAGIC_NUMBER)
	{

		while (1) {
			;
		}
	}
	
	sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
	sSpiInformation.SPIRxHandler(sSpiInformation.pRxPacket + SPI_HEADER_SIZE);
}


//*****************************************************************************
//
//! Returns state of IRQ pin
//!
//
//*****************************************************************************

long ReadWlanInterruptPin(void)
{
	return(hw_digital_read(CC3000_nIRQ));
}


void WlanInterruptEnable()
{

	hw_interrupt_listen(0, CC3K_IRQ, TM_INTERRUPT_MODE_FALLING); //Attaches Pin 2 to interrupt 1
	hw_interrupt_callback_attach(0, SPI_IRQCallback);
}


void WlanInterruptDisable()
{
	hw_interrupt_callback_detach(0);	//Detaches Pin 3 from interrupt 1
}
// int STATE = 0;

// Callback with two arguments for tm_interrupt.
void SPI_IRQCallback(int first, int second)
{
	(void) first;
	(void) second;
	SPI_IRQ();
}

void SPI_IRQ()
{
//	TM_DEBUG("SPI_IRQ called: %ul", sSpiInformation.ulSpiState);

	// print_spi_state();
	if (sSpiInformation.ulSpiState == eSPI_STATE_POWERUP)
	{

		//This means IRQ line was low call a callback of HCI Layer to inform on event 
		sSpiInformation.ulSpiState = eSPI_STATE_INITIALIZED;

	}
	else if (sSpiInformation.ulSpiState == eSPI_STATE_IDLE)
	{

		sSpiInformation.ulSpiState = eSPI_STATE_READ_IRQ;
		// THESE MIGHT NOT BE USEFUL:
		csn(HW_LOW);
		//
		// Wait for TX/RX Compete which will come as DMA interrupt
		// 
		SpiReadHeader();

		sSpiInformation.ulSpiState = eSPI_STATE_READ_EOT;
		
		SSIContReadOperation();
		
	}
	else if (sSpiInformation.ulSpiState == eSPI_STATE_WRITE_IRQ)
	{
//		TM_DEBUG("SPI write IRQ called");
		SpiWriteDataSynchronous(sSpiInformation.pTxPacket, 
			sSpiInformation.usTxPacketLength);
//		TM_DEBUG("SPI is done writing data");
		sSpiInformation.ulSpiState = eSPI_STATE_IDLE;
	}

}

void print_spi_state(void)
{
	if (DEBUG_MODE)
	{

		switch (sSpiInformation.ulSpiState)
		{
			case eSPI_STATE_POWERUP:
				TM_DEBUG("POWERUP");
				break;
			case eSPI_STATE_INITIALIZED:
				TM_DEBUG("INITIALIZED");
				break;
			case eSPI_STATE_IDLE:
				TM_DEBUG("IDLE");
				break;
			case eSPI_STATE_WRITE_IRQ:
				TM_DEBUG("WRITE_IRQ");
				break;
			case eSPI_STATE_WRITE_FIRST_PORTION:
				TM_DEBUG("WRITE_FIRST_PORTION");
				break;
			case eSPI_STATE_WRITE_EOT:
				TM_DEBUG("WRITE_EOT");
				break;
			case eSPI_STATE_READ_IRQ:
				TM_DEBUG("READ_IRQ");
				break;
			case eSPI_STATE_READ_FIRST_PORTION:
				TM_DEBUG("READ_FIRST_PORTION");
				break;
			case eSPI_STATE_READ_EOT:
				TM_DEBUG("STATE_READ_EOT");
				break;
			default:
				break;
		}
	}

	return;
}


void WriteWlanPin( unsigned char val )
{
	if (val)
	{
		hw_digital_write(HOST_VBAT_SW_EN, 1);
	}
	else
	{
		hw_digital_write(HOST_VBAT_SW_EN, 0);
	}

}


//*****************************************************************************
//
//  The function handles asynchronous events that come from CC3000 device 
//!		  
//
//*****************************************************************************

void CC3000_UsynchCallback(long lEventType, char * data, unsigned char length)
{
	(void) length;
	
	// if (DEBUG_MODE)
	// {
	// 	TM_DEBUG("CC3000_UsynchCallback");
	// }

	if (lEventType == HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE)
	{
		ulSmartConfigFinished = 1;
		ucStopSmartConfig     = 1;  
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_CONNECT)
	{
		ulCC3000Connected = 1;
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DISCONNECT)
	{		
		ulCC3000Connected = 0;
		ulCC3000DHCP      = 0;
		ulCC3000DHCP_configured = 0;
		printOnce = 1;
		
	}
	
	if (lEventType == HCI_EVNT_WLAN_UNSOL_DHCP)
	{

		// Notes: 
		// 1) IP config parameters are received swapped
		// 2) IP config parameters are valid only if status is OK, i.e. ulCC3000DHCP becomes 1
		
		// only if status is OK, the flag is set to 1 and the addresses are valid
		if ( *(data + NETAPP_IPCONFIG_MAC_OFFSET) == 0)
		{
			 TM_DEBUG("DHCP Ip: %d.%d.%d.%d", data[3], data[2], data[1], data[0]);
			 hw_digital_write(CC3K_CONN_LED, 1);

			ulCC3000DHCP = 1;
		}
		else
		{
			ulCC3000DHCP = 0;
			TM_DEBUG("DHCP failed. Try reconnecting.");
			hw_digital_write(CC3K_CONN_LED, 0);
		}
	}
	
	if (lEventType == HCI_EVENT_CC3000_CAN_SHUT_DOWN)
	{
		OkToDoShutDown = 1;
	}
}



// *****************************************************************************

// ! This function enter point for write flow
// !
// !  \param  SSIContReadOperation
// !
// !  \return none
// !
// !  \brief  The function triggers a user provided callback for 

// *****************************************************************************

void SSIContReadOperation(void)
{
//	TM_DEBUG("SSIContReadOp");
	
	//
	// The header was read - continue with  the payload read
	//
	if (!SpiReadDataCont())
	{
		
//		TM_DEBUG("triggering RX processing");
		//
		// All the data was read - finalize handling by switching to teh task
		//	and calling from task Event Handler
		//
		SpiTriggerRxProcessing();
	}
}

/**
 * Smart Config
 */

// void FinishSmartConfig(void) {
// 	mdnsAdvertiser(1,device_name,strlen(device_name));
// }
#define CC3000_UNENCRYPTED_SMART_CONFIG 1
 void StartSmartConfig(void)
 {
 	TM_DEBUG("Start Smart Config");

 	ulSmartConfigFinished = 0;
 	ulCC3000Connected = 0;
 	ulCC3000DHCP = 0;
 	OkToDoShutDown=0;

 	// Reset all the previous configuration
 	wlan_ioctl_set_connection_policy(0, 0, 0);
 	wlan_ioctl_del_profile(255);

 	TM_DEBUG("Deleted profiles");
 	//Wait until CC3000 is disconnected
 	while (ulCC3000Connected == 1)
 	{
// 		delayMicroseconds(100);
 		CC_BLOCKS();
 		fakeWait(100000);
 	}

 	// Trigger the Smart Config process
 	// Start blinking LED6 during Smart Configuration process
 	hw_digital_write(CC3K_CONN_LED, 1);
 	wlan_smart_config_set_prefix((char*)aucCC3000_prefix);
 	hw_digital_write(CC3K_CONN_LED, 0);

 	TM_DEBUG("starting config");
 	// Start the SmartConfig start process
 	wlan_smart_config_start(0);

 	hw_digital_write(CC3K_CONN_LED, 1);

 	// Wait for Smartconfig process complete
 	while (ulSmartConfigFinished == 0)
 	{
 		CC_BLOCKS();
 		fakeWait(1000000);

 		hw_digital_write(CC3K_CONN_LED, 0);

 		fakeWait(1000000);

 		hw_digital_write(CC3K_CONN_LED, 1);

 	}
 	TM_DEBUG("finishd config");
 	hw_digital_write(CC3K_CONN_LED, 1);

 #ifndef CC3000_UNENCRYPTED_SMART_CONFIG
 	// create new entry for AES encryption key
 	nvmem_create_entry(NVMEM_AES128_KEY_FILEID,16);

 	// write AES key to NVMEM
 	aes_write_key((unsigned char *)(&smartconfigkey[0]));

 	// Decrypt configuration information and add profile
 	wlan_smart_config_process();
 #endif

 	// Configure to connect automatically to the AP retrieved in the
 	// Smart config process
 	TM_DEBUG("reset policy");
 	wlan_ioctl_set_connection_policy(0, 0, 1);

 	// reset the CC3000
 	wlan_stop();

 	fakeWait(6000000);
 	hw_digital_write(CC3K_CONN_LED, 0);
 	TM_DEBUG("Config done");
 	wlan_start(0);

 	// Mask out all non-required events
 	wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE|HCI_EVNT_WLAN_UNSOL_INIT|HCI_EVNT_WLAN_ASYNC_PING_REPORT);
 	smartconfig_process = 0;
 }

