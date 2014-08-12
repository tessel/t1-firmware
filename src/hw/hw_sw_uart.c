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
#define ADJUST      (1<<30)
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
static volatile signed long int edge_last, edge_sample, edge_current, edge_stop;
static volatile unsigned short int swu_rx_fifo[RXBUFF_LEN];
//definitions common for transmitting and receiving data
static volatile unsigned long int swu_status;

void swu_tx_chr(const unsigned char out_char, SWUartBitLength Bit_Length);

/*********************************************************
** Local Functions                                      **
*********************************************************/
static void swu_setup_tx(SWUartBitLength Bit_Length); //tx param processing
void swu_rx_callback(void); //__weak
// void swu_tx_callback(void); //__weak

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
static void swu_setup_tx(SWUartBitLength Bit_Length) {
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
    edge[0] = Bit_Length; //at least 1 falling edge...
    cnt_edges = 1; //... because of the START bit
    bit = 1; //set the bit counter
    reference = 0x00000000; //init ref is 0 (start bit)
    mask = 1 << 1; //prepare the mask
    delta_edges = Bit_Length; //next edge at least 1 bit away
    while (bit != 10) { //until all bits are examined
      if ((ext_data & mask) == (reference & mask)) { //bit equal to the reference?
        delta_edges += Bit_Length; //bits identical=>update length
      } //...
      else { //bits are not the same:
        edge[cnt_edges] = //store new...
          edge[cnt_edges - 1] + delta_edges; //... edge data
        reference = ~reference; //update the reference
        delta_edges = Bit_Length; //reset delta_ to 1 bit only
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
    reference = LPC_TIMER1->TC + Bit_Length; //get the reference from TIMER0
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
void swu_tx_str(unsigned char const* ptr_out, uint32_t data_size, SWUartBitLength Bit_Length) {
  while(data_size > 0){
    swu_tx_chr(*ptr_out, Bit_Length); //...put the char in tx FIFO...
    ptr_out++; //...move to the next char...
    data_size--;
  }
  return; //return from the routine
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
void swu_tx_chr(const unsigned char out_char, SWUartBitLength Bit_Length) {
  //write access to tx FIFO begin
  while (swu_tx_cnt == TXBUFF_LEN)
    ; //wait if the tx FIFO is full
  tx_fifo_wr_ind++; //update the write pointer...
  if (tx_fifo_wr_ind == TXBUFF_LEN) //...
    tx_fifo_wr_ind = 0; //...
  swu_tx_fifo[tx_fifo_wr_ind] = out_char; //put the char into the FIFO
  swu_tx_cnt++; //update no.of chrs in the FIFO
  if ((swu_status & TX_ACTIVE) == 0)
    swu_setup_tx(Bit_Length); //start tx if tx is not active

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
  if (swu_rx_cnt != 0) { //update the rx indicator...
    rx_fifo_rd_ind++; //... if data are present...
    if (rx_fifo_rd_ind == RXBUFF_LEN)
      rx_fifo_rd_ind = 0; //...
    swu_rx_cnt--; //...
  }
  if ((swu_rx_fifo[rx_fifo_rd_ind] & 0x0100) == 0) //update...
    swu_rx_chr_fe = 0; //... the framing error...
  else {
    //... indicator...
    swu_rx_chr_fe = 1; //...
    // hw_digital_write(CC3K_ERR_LED, 1);
    hw_digital_write(LED2, 1);
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
void swu_isr_tx(LPC_TIMERn_Type* const TX_ISR_TIMER, SWUartBitLength Bit_Length) {
    TX_ISR_TIMER->IR = 1 << 2; //0x08; //clear the MAT2 flag
    if (edge_index == char_end_index) //the end of the char:
    {
      TX_ISR_TIMER->MCR &= ~(7 << 6); //MR2 impacts T0 ints no more
      swu_tx_cnt--; //update no.of chars in tx FIFO
      if (tx_fifo_wr_ind != tx_fifo_rd_ind) //if more data pending...
        swu_setup_tx(Bit_Length); //... spin another transmission
      else {
        swu_status &= ~TX_ACTIVE; //no data left=>turn off the tx
        // swu_tx_callback();
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
  SWUartBitLength Bit_Length, SWUartStopBitSample Stop_Bit_Sample) {


  signed long int edge_temp;
  //sw uart receive isr code begin
  if (0 != (RX_ISR_TIMER->IR & (IR_CLEAR_CAP_2))) //capture interrupt occured on CAP2
  {
    LPC_GPIO_PORT->DIR[3] |= (1 << 10);
    RX_ISR_TIMER->IR = IR_CLEAR_CAP_2;//0x10; //edge detcted=>clear CAP2 flag
    //change the targeted edge
    RX_ISR_TIMER->CCR = (1<< 8) | ((3 << 6) - (RX_ISR_TIMER->CCR & (3 << 6)));//0x0004 | (0x0003 - (RX_ISR_TIMER->CCR & 0x0003));
    if ((swu_status & RX_ACTIVE) == 0) { //sw UART not active (start):
      edge_last = (signed long int) RX_ISR_TIMER->CR[2]; //initialize the last edge
      edge_sample = edge_last + (Bit_Length >> 1); //initialize the sample edge
      if (edge_sample < edge_last) //adjust the sample edge...
        edge_sample |= ADJUST; //... if needed
      swu_bit = 0; //rx bit is 0 (a start bit)
      // use mat1 to match stop bit timing
      RX_ISR_TIMER->IR = 0x01; //clear MAT1 int flag
      edge_stop = edge_sample + Stop_Bit_Sample; //estimate the end of the byte
      if (edge_stop < edge_last) //adjust the end of byte...
        edge_stop |= ADJUST; //... if needed
      RX_ISR_TIMER->MR[1] = edge_stop; //set MR1 (stop bit center)
      RX_ISR_TIMER->MCR = RX_ISR_TIMER->MCR | (1 << 3); //int on MR1
      cnt = 9; //initialize the bit counter
      swu_status |= RX_ACTIVE; //update the swu status
      swu_rbr = 0x0000; //reset the sw rbr
      swu_rbr_mask = 0x0001; //initialize the mask
    } else {
      //reception in progress:
      edge_current = (signed long int) RX_ISR_TIMER->CR[2]; //initialize the current edge
      if (edge_current < edge_last) //adjust the current edge...
        edge_current |= ADJUST; //... if needed
      while (edge_current > edge_sample) { //while sampling edge is within
        if (cnt_bits != 0) {
          if (swu_bit != 0) //update data...
            swu_rbr |= swu_rbr_mask; //...
          swu_rbr_mask = swu_rbr_mask << 1; //update mask
        }
        cnt_bits++; //update the bit count
        edge_temp = edge_last + Bit_Length; //estimate the last edge
        if (edge_temp < edge_last) //adjust...
          edge_last = edge_temp | ADJUST; //... the last edge...
        else
          //... if...
          edge_last = edge_temp; //... needed
        edge_temp = edge_sample + Bit_Length; //estimate the sample edge
        if (edge_temp < edge_sample) //adjust...
          edge_sample = edge_temp | ADJUST; //... the sample edge...
        else
          //... if...
          edge_sample = edge_temp; //... needed
        cnt--; //update the no of rcved bits
      }
      swu_bit = 1 - swu_bit; //change the received bit
    }
    LPC_GPIO_PORT->DIR[3] &= ~(1 << 10);
  }

  if (0 != (RX_ISR_TIMER->IR & 0x02)) //stop bit timing matched:
  {
    LPC_GPIO_PORT->DIR[3] |= (1 << 10);
    RX_ISR_TIMER->IR = 0x02; //clear MR1 flag
    if (cnt != 0) { //not all data bits received...
      swu_rbr = swu_rbr << cnt; //... make space for the rest...
      if (swu_bit != 0)
        swu_rbr += ALL1 << (8 - cnt); //... add needed 1(s)...
    } //...
    swu_rbr &= 0x00FF; //extract data bits only
    if (swu_bit == 0) //if the stop bit was 0 =>
      swu_rbr |= 0x00000100; //... framing error!
    swu_status &= ~RX_ACTIVE; //sw UART not active any more
    cnt_bits = 0; //reset the rx bit count
    if (swu_rx_cnt != RXBUFF_LEN) //store the rcved character...
    {
      swu_rx_cnt++; //... into the sw UART...
      rx_fifo_wr_ind++; //... rx FIFO
      if (rx_fifo_wr_ind == RXBUFF_LEN)
        rx_fifo_wr_ind = 0; //...
      swu_rx_fifo[rx_fifo_wr_ind] = swu_rbr; //...
      if (swu_rx_cnt >= swu_rx_trigger)
        swu_rx_callback(); //rx 'isr' trig excded
    } else {
      swu_status |= RX_OVERFLOW; //rx FIFO full => overflow
    }
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


// int numInBuff = 0;
// unsigned char buff[5];

void TIMER1_IRQHandler(void)
{
  // use this to check for when IRQs are hit
  hw_digital_write(A_G1, 1);
  hw_digital_write(A_G1, 0);
  //tx stuff
	if (LPC_TIMER1->IR & (1<<2))  // check match reg of match reg 2
	{
		swu_isr_tx(LPC_TIMER1, TM_SW_UART_115200);
	} 
  // rx flag for cap2 and mat1
  if (((LPC_TIMER1->IR & (1<<6)) || (LPC_TIMER1->IR & (1<<1)))){ // check irq of cap reg 2 or match timer 1
    swu_isr_rx(LPC_TIMER1, TM_SW_UART_9600, TM_SW_UART_9600_STOP);
  }
	
  hw_digital_write(A_G1, 1);

}

// void swu_tx_callback(void)
// {
//   // clear the buffer
//   memset(buff, 0, 5);
//   numInBuff = 0;
// }

void swu_rx_callback(void)
{
  //append flag bit to character
  rxData =0x100 + swu_rx_chr();

  gps_consume((unsigned char) rxData & 0xFF);

  // if (SW_UART_RECV_POS < SW_UART_BUFF_LEN) { // if the ending rx flag is set save this char
  //   unsigned char buffChar = (unsigned char) rxData & 0xFF;
  //   SW_UART_BUFF[SW_UART_RECV_POS] = buffChar;
  //   rxData=0; // reset flag
  //   SW_UART_RECV_POS++;
  //   hw_digital_write(LED1, 0);
  //   // consume the character
  //   gps_consume(buffChar);
  // } else {
  //   // set the ready flag
  //   // SW_UART_RDY = 1;
  //   hw_digital_write(LED1, 1);
  //   SW_UART_RECV_POS = 0;
  // }

  // start timer for UART receive timeout on match register 3
  // LPC_TIMER1->MR[0] = INITIAL_TIME; //TIMER0_32 counts 0 - 0x3FFFFFFF
  // LPC_TIMER1->MCR = 2;//MCR_RESET_MAT_2; //1<<7; //reset TIMER1 on MR2
  // LPC_TIMER1->EMR = EMR_EN_MAT_2; //1<<2; //enable timer 1 match 2

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
void swu_init() {
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
  LPC_TIMER1->MCR = 2;//MCR_RESET_MAT_2; //1<<7; //reset TIMER1 on MR2
  LPC_TIMER1->EMR = EMR_EN_MAT_2; //1<<2; //enable timer 1 match 2

  LPC_TIMER1->IR = IR_CLEAR_CAP_2;//1 << 6; // //clear CAP1.2 flag
  LPC_TIMER1->CCR = CCR_INT_CAP_2; //3 << 7; //int on CAP1.2 falling edge

  LPC_TIMER1->TCR = 0x01; //let TIMER0_32 run
  NVIC_EnableIRQ(TIMER1_IRQn); //Enable the TIMER0_32 Interrupt
  cnt_bits = 0; //reset the rx bit count
}

void sw_uart_gps_init(void) {
  // inits gps with sw uart
  unsigned char gps_init_buff[32] = {0xA0, 0xA2, 0x00, 0x18, 0x81, 0x02, 0x01, 0x01,
    0x00, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x25, 0x80, 0x01, 0x3A, 0xB0, 0xB3};

  swu_tx_str(gps_init_buff, sizeof(gps_init_buff), TM_SW_UART_115200);
}

void sw_uart_test(void) {
  swu_init();
  // unsigned char data[4] = {0x56, 0x00, 0x01, 0x11};
  // while (1) {

  //   // if (rxData & 0x100 && numInBuff < 5) { // if the ending rx flag is set save this char
  //   //   unsigned char buffChar = (unsigned char) rxData & 0xFF;
  //   //   buff[numInBuff] = buffChar;
  //   //   rxData=0; // reset flag
  //   //   numInBuff++;
  //   //   hw_digital_write(LED2, 1);
  //   // } else 
  //   if (SW_UART_RDY) {
  //     // write out the buffer we got to tx again
  //     swu_tx_str(SW_UART_BUFF, sizeof(SW_UART_BUFF));
  //     // hw_digital_write(LED2, 1);
  //     SW_UART_RECV_POS = 0;
  //     SW_UART_RDY = 0;
  //   }


  // }
}