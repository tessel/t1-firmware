// Modified from the AN10955 app note
// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#include "hw.h"
#include "tm.h"
#include "tessel.h"
#include "gps-nmea.h"

/*********************************************************
** Macro Definitions                                 	**
*********************************************************/
#define RX_OVERFLOW 4
#define RX_ACTIVE   2
#define TX_ACTIVE   1
#define ALL1        0x000000FF

#define IR_CLEAR_CAP_2		1 << 6
#define CCR_INT_CAP_2		3 << 7
#define MCR_RESET_MAT_2 	1 << 7
#define EMR_EN_MAT_2		1 << 2
#define INITIAL_TIME		0x3FFFFFFF

volatile int SW_UART_RDY = 0;
volatile int SW_UART_RECV_POS = 0;
/*********************************************************
** Global Variables                                     **
*********************************************************/
static volatile unsigned char cnt_edges; //no of char edges
static volatile unsigned char edge_index; //edge index for output
static volatile unsigned char swu_tx_st; //sw UART status
static volatile unsigned long int edge[11]; //array of edges
static volatile unsigned char last_edge_index, char_end_index; //last two events index
//software UART Tx related definitions
static volatile unsigned long int tx_fifo_wr_ind, tx_fifo_rd_ind;
static volatile signed long int swu_tx_cnt, swu_tx_trigger;
static volatile unsigned short int swu_tx_fifo[TXBUFF_LEN];
//software UART Rx related definitions
static volatile unsigned long int rx_fifo_wr_ind, rx_fifo_rd_ind;
static volatile signed long int swu_rx_cnt, swu_rx_trigger;
static volatile unsigned char swu_bit, cnt, cnt_bits, swu_rx_chr_fe;
static volatile unsigned long int swu_rbr, swu_rbr_mask;
static volatile unsigned long int edge_last, edge_sample, edge_current, edge_stop;
static volatile unsigned short int swu_rx_fifo[RXBUFF_LEN];
//definitions common for transmitting and receiving data
static volatile unsigned long int swu_status;

void swu_tx_chr(const unsigned char out_char, hw_swuart_bitlength_t bit_length);

/*********************************************************
** Local Functions                                      **
*********************************************************/
static void swu_setup_tx(hw_swuart_bitlength_t bit_length); //tx param processing
void swu_rx_callback(void); //__weak
void swu_tx_callback(void); //__weak

static volatile unsigned short rxData;


/******************************************************************************
** Function name:   swu_setup_tx
**
** Descriptions:
**
** This routine prepares an array of toggle points that will be used to generate
** a waveform for the currently selected character in the software UART transmission
** FIFO; at the end this routine starts the transmission itself.
**
** parameters:	    None
** Returned value:  None
**
******************************************************************************/
static void swu_setup_tx(hw_swuart_bitlength_t bit_length) {
  unsigned char bit, i;
  unsigned long int ext_data, delta_edges, mask, reference;
  if (tx_fifo_wr_ind != tx_fifo_rd_ind) //data to send, proceed
  {
    swu_status |= TX_ACTIVE; //sw uart tx is active
    tx_fifo_rd_ind++; //update the tx fifo ...
    if (tx_fifo_rd_ind == TXBUFF_LEN) //read index...
      tx_fifo_rd_ind = 0; //...
    ext_data = (unsigned long int) swu_tx_fifo[tx_fifo_rd_ind]; //read the data
    ext_data = 0xFFFFFE00 | (ext_data << 1); //prepare the pattern
    edge[0] = bit_length; //at least 1 falling edge...
    cnt_edges = 1; //... because of the START bit
    bit = 1; //set the bit counter
    reference = 0x00000000; //init ref is 0 (start bit)
    mask = 1 << 1; //prepare the mask
    delta_edges = bit_length; //next edge at least 1 bit away
    while (bit != 10) { //until all bits are examined
      if ((ext_data & mask) == (reference & mask)) { //bit equal to the reference?
        delta_edges += bit_length; //bits identical=>update length
      } //...
      else { //bits are not the same:
        edge[cnt_edges] = //store new...
          edge[cnt_edges - 1] + delta_edges; //... edge data
        reference = ~reference; //update the reference
        delta_edges = bit_length; //reset delta_ to 1 bit only
        cnt_edges++; //update the edges counter
      }
      mask = mask << 1; //update the mask
      bit++; //move on to the next bit
    }
    edge[cnt_edges] = //add the stop bit end...
      edge[cnt_edges - 1] + delta_edges; //... to the list
    cnt_edges++; //update the number of edges
    last_edge_index = cnt_edges - 2; //calculate the last edge index
    char_end_index = cnt_edges - 1; //calc. the character end index

    edge_index = 0; //reset the edge index
    reference = LPC_TIMER1->TC + bit_length; //get the reference from TIMER0
    for (i = 0; i != cnt_edges; i++) //recalculate toggle points...
      edge[i] = (edge[i] + reference) //... an adjust for the...
        & INITIAL_TIME; //... timer range
    LPC_TIMER1->MR[2] = edge[0]; //load MR2
    LPC_TIMER1->MCR = LPC_TIMER1->MCR | (1 << 6); //enable interrupt on MR2 match
    LPC_TIMER1->EMR = LPC_TIMER1->EMR | (3 << 8); //enable toggle on MR2 match
  }
  return; //return from the routine
}

/******************************************************************************
** Function name:		swu_tx_str
**
** Descriptions:
**
** This routine transfers a string of characters one by one into the
** software UART tx FIFO.
**
** parameters:		string pointer
** Returned value:		None
**
******************************************************************************/
int hw_swuart_transmit(unsigned char const* ptr_out, uint32_t data_size, hw_swuart_bitlength_t bit_length) {
  // This is only needed for TX, RX will mess up if this bit is set
  LPC_TIMER1->MCR = 2;// TC will be reset if MR0 matches it.

  while(data_size > 0){
    swu_tx_chr(*ptr_out, bit_length); //...put the char in tx FIFO...
    ptr_out++; //...move to the next char...
    data_size--;
  }
  return 0; //return from the routine
}

/******************************************************************************
** Function name:		swu_tx_chr
**
** Descriptions:
**
** This routine puts a single character into the software UART tx FIFO.
**
** parameters:		char
** Returned value:		None
**
******************************************************************************/
void swu_tx_chr(const unsigned char out_char, hw_swuart_bitlength_t bit_length) {
  //write access to tx FIFO begin
  while (swu_tx_cnt == TXBUFF_LEN)
    ; //wait if the tx FIFO is full
  tx_fifo_wr_ind++; //update the write pointer...
  if (tx_fifo_wr_ind == TXBUFF_LEN) //...
    tx_fifo_wr_ind = 0; //...
  swu_tx_fifo[tx_fifo_wr_ind] = out_char; //put the char into the FIFO
  swu_tx_cnt++; //update no.of chrs in the FIFO
  if ((swu_status & TX_ACTIVE) == 0)
    swu_setup_tx(bit_length); //start tx if tx is not active

  return; //return from the routine
}

/******************************************************************************
** Function name:		swu_rx_chr
**
** Descriptions:
**
** This routine reads a single character from the software UART rx FIFO.
** If no new data is available, it returns the last one read;
** The framing error indicator is updated, too.
**
** parameters:		None
** Returned value:		char
**
******************************************************************************/
unsigned char swu_rx_chr(void) {
  rx_fifo_rd_ind++; //... if data are present...
  if (rx_fifo_rd_ind == RXBUFF_LEN)
    rx_fifo_rd_ind = 0; //...
  if ((swu_rx_fifo[rx_fifo_rd_ind] & 0x0100) == 0) //update...
    swu_rx_chr_fe = 0; //... the framing error...
  else {
    //... indicator...
    swu_rx_chr_fe = 1; //...
  }
  swu_status &= ~RX_OVERFLOW; //clear the overfloe flag
  return ((unsigned char) (swu_rx_fifo[rx_fifo_rd_ind] & 0x00FF)); //return data
}

/******************************************************************************
** Function name: swu_isr_tx
**
** Descriptions:
**
** Handle timer output toggle events. If there are additional edge events
** in the edge[] array, continue to load the match register with them. If
** the last edge has been toggled, check for additional characters to load
** from FIFO, otherwise terminate transmission of characters.
**
** parameters:  TX_ISR_TIMER pointer to timer resource to enable
**              increased portability.
** Returned value:  None
**
******************************************************************************/
void swu_isr_tx(LPC_TIMERn_Type* const TX_ISR_TIMER, hw_swuart_bitlength_t bit_length) {
    TX_ISR_TIMER->IR = 1 << 2; //0x08; //clear the MAT2 flag
    if (edge_index == char_end_index) //the end of the char:
    {
      TX_ISR_TIMER->MCR &= ~(7 << 6); //MR2 impacts T0 ints no more
      swu_tx_cnt--; //update no.of chars in tx FIFO
      if (tx_fifo_wr_ind != tx_fifo_rd_ind) //if more data pending...
        swu_setup_tx(bit_length); //... spin another transmission
      else {
        swu_status &= ~TX_ACTIVE; //no data left=>turn off the tx
        swu_tx_callback();
      }
    } else {
      if (edge_index == last_edge_index) //is this the last toggle?
        TX_ISR_TIMER->EMR &= ~(7 << 7); //0x000003FF; //no more toggle on MAT2
      edge_index++; //update the edge index
      TX_ISR_TIMER->MR[2] = edge[edge_index]; //prepare the next toggle event
    }

}

/******************************************************************************
** Function name: swu_isr_rx
**
** Descriptions:
**
** Look for start bit falling edge. If found begin comparing edge events to
** decode the received character, followed by stop bit detection routine.
**
** parameters:  TX_ISR_TIMER pointer to timer resource to enable
**              increased portability.
** Returned value:  None
**
******************************************************************************/
void swu_isr_rx(LPC_TIMERn_Type* const RX_ISR_TIMER, 
  hw_swuart_bitlength_t bit_length, hw_swuart_stopbit_t Stop_Bit_Sample) {


  //sw uart receive isr code begin
  if (0 != (RX_ISR_TIMER->IR & (IR_CLEAR_CAP_2))) //capture interrupt occured on CAP2
  {
    LPC_GPIO_PORT->DIR[3] |= (1 << 10);
    RX_ISR_TIMER->IR = IR_CLEAR_CAP_2;//0x10; //edge detcted=>clear CAP2 flag
    //change the targeted edge & clear lower bits
    RX_ISR_TIMER->CCR &= ~63;
    RX_ISR_TIMER->CCR ^= (1 << 6) ^ (1 << 7);
    if ((swu_status & RX_ACTIVE) == 0) { //sw UART not active (start):
      edge_last = (unsigned long int) RX_ISR_TIMER->CR[2]; //initialize the last edge
      // add bit_length/2 to the current location because we're halfway in between the sample right now
      edge_sample = edge_last + (bit_length >> 1); //initialize the sample edge
      swu_bit = 0; //rx bit is 0 (a start bit)
      // use mat1 to match stop bit timing
      RX_ISR_TIMER->IR = 0x01; //clear MAT1 int flag
      edge_stop = edge_sample + Stop_Bit_Sample; //estimate the end of the byte
      RX_ISR_TIMER->MR[1] = edge_stop; //set MR1 (stop bit center)
      RX_ISR_TIMER->MCR = RX_ISR_TIMER->MCR | (1 << 3); //int on MR1
      cnt = 9; //initialize the bit counter
      swu_status |= RX_ACTIVE; //update the swu status
      swu_rbr = 0x0000; //reset the sw rbr
      swu_rbr_mask = 0x0001; //initialize the mask

    } else {
      //reception in progress:
      edge_current = (unsigned long int) RX_ISR_TIMER->CR[2]; //initialize the current edge
      
      while (edge_current > edge_sample) { //while sampling edge is within
        if (cnt_bits != 0) {
          if (swu_bit != 0) //update data...
            swu_rbr |= swu_rbr_mask; //...
          swu_rbr_mask = swu_rbr_mask << 1; //update mask
        }
        cnt_bits++; //update the bit count
        edge_last = edge_last + bit_length; //estimate the last edge
        edge_sample = edge_sample + bit_length; //estimate the sample edge

        cnt--; //update the no of rcved bits
      }
      swu_bit = 1 - swu_bit; //change the received bit
    }
    LPC_GPIO_PORT->DIR[3] &= ~(1 << 10); // set gpio as input
  }

  //stop bit timing matched or we messed up and already passed the stop timer
  if (0 != (RX_ISR_TIMER->IR & 0x02) || RX_ISR_TIMER->TC >= RX_ISR_TIMER->MR[1] ) 
  {
    LPC_GPIO_PORT->DIR[3] |= (1 << 10);
    RX_ISR_TIMER->IR = 0x02; //clear MR1 flag
    if (cnt != 0) { //not all data bits received...
      swu_rbr = swu_rbr << cnt; //... make space for the rest...
      if (swu_bit != 0)
        swu_rbr += ALL1 << (8 - cnt); //... add needed 1(s)...
    }
    swu_rbr &= 0x00FF; //extract data bits only
    if (swu_bit == 0) //if the stop bit was 0 =>
      swu_rbr |= 0x00000100; //... framing error!
    swu_status &= ~RX_ACTIVE; //sw UART not active any more
    cnt_bits = 0; //reset the rx bit count
    rx_fifo_wr_ind++; //... rx FIFO
    if (rx_fifo_wr_ind == RXBUFF_LEN)
      rx_fifo_wr_ind = 0; //...
    swu_rx_fifo[rx_fifo_wr_ind] = swu_rbr; //...
    swu_rx_callback(); //rx 'isr' trig excded

    RX_ISR_TIMER->MCR &= ~(7 << 3);//MR1 impacts timer1 no more
    LPC_GPIO_PORT->DIR[3] &= ~(1 << 10);
  }
  //sw uart receive isr code end
}


// UART Configuration structure variable
UART_CFG_Type UARTConfigStruct;
uint32_t MatchCount;

//timer init
TIM_TIMERCFG_Type TIM_ConfigStruct;
TIM_MATCHCFG_Type TIM_MatchConfigStruct ;
TIM_CAPTURECFG_Type TIM_CaptureConfigStruct;
uint8_t volatile timer1_flag = FALSE;

void TIMER1_IRQHandler(void)
{
  // use this to check for when IRQs are hit
  //tx stuff
	if (LPC_TIMER1->IR & (1<<2))  // check match reg of match reg 2
	{
		swu_isr_tx(LPC_TIMER1, TM_SW_UART_115200);
	} 

  // rx flag for cap2 and mat1
  // cap2 = UART bit changed
  // mat1 = we hit the stop bit
  if (((LPC_TIMER1->IR & (1<<6)) || (LPC_TIMER1->IR & (1<<1)))){ 
    swu_isr_rx(LPC_TIMER1, TM_SW_UART_9600, TM_SW_UART_9600_STOP);
  }

}

void swu_tx_callback(void)
{
  // stop TC from being reset if MR[0] matches. enables RX registers to go past MR[0] values
  LPC_TIMER1->MCR ^= 2;
}

void swu_rx_callback(void)
{
  //append flag bit to character
  rxData =0x100 + swu_rx_chr();

  unsigned char buffChar = (unsigned char) rxData & 0xFF;
  gps_consume(buffChar);

  if (SW_UART_RECV_POS < SW_UART_BUFF_LEN) { // if the ending rx flag is set save this char
    SW_UART_BUFF[SW_UART_RECV_POS] = buffChar;
    rxData=0; // reset flag
    SW_UART_RECV_POS++;
  } else {
    // set the ready flag
    SW_UART_RDY = 1;
    SW_UART_RECV_POS = 0;
  }

  return;
}
/******************************************************************************
** Function name: 	swu_init
**
** Description:		Initialize variables used by software UART.
** 					Configure specific IO pins.
** 					Setup and start timer running.
**
** parameters:  	UART_TIMER  - pointer to timer resource to enable configuration.
** Returned value:  None
**
******************************************************************************/
int hw_swuart_enable() {
  //setup the software uart
  swu_tx_cnt = 0; //no data in the swu tx FIFO
  tx_fifo_wr_ind = 0; //last char written was on 0
  tx_fifo_rd_ind = 0; //last char updated was on 0
  swu_rx_trigger = 1; //>=1 char gnrts a rx interrupt
  swu_status = 0; //neither tx nor rx active

  scu_pinmux(0x0B ,1 , MD_PDN, FUNC5); 	// CTOUT_6 Ñ SCT output 6. Match output 2 of timer 1.
  scu_pinmux(7 ,2 , MD_PLN_FAST, FUNC1); // CTIN_4 Ñ SCT input 4. Capture input 2 of timer 1.

  // Initialize timer 1, prescale count time of 100uS
  CGU_ConfigPWR(CGU_PERIPHERAL_TIMER1, ENABLE);

  LPC_TIMER1->TCR = 0x00;
  LPC_TIMER1->TCR = 0x02; //reset counter
  LPC_TIMER1->TCR = 0x00; //release the reset

  LPC_TIMER1->IR = 0x0FF; //clear all TIMER0 flags
  LPC_TIMER1->PR = 0x00000000; //no prescaler
  LPC_TIMER1->MR[0] = INITIAL_TIME; //TIMER0_32 counts 0 - 0x3FFFFFFF
  LPC_TIMER1->EMR = EMR_EN_MAT_2; //1<<2; //enable timer 1 match 2

  LPC_TIMER1->IR = IR_CLEAR_CAP_2;//1 << 6; // //clear CAP1.2 flag
  LPC_TIMER1->CCR = CCR_INT_CAP_2; //3 << 7; //int on CAP1.2 falling edge

  LPC_TIMER1->TCR = 0x01; //let TIMER0_32 run
  NVIC_EnableIRQ(TIMER1_IRQn); //Enable the TIMER0_32 Interrupt
  cnt_bits = 0; //reset the rx bit count

  return 0;
}

int hw_swuart_disable(){
  NVIC_DisableIRQ(TIMER1_IRQn);
  return 0;
}
