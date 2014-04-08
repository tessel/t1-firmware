// sw uart for port bank c
#include "hw.h"
#include "variant.h"
#include "lpc18xx_uart.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_libcfg.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_timer.h"

/*********************************************************
** Macro Definitions                                         **
*********************************************************/
#define RX_OVERFLOW 4
#define RX_ACTIVE   2
#define TX_ACTIVE   1
#define ADJUST      (1<<30)
#define ALL1        0x000000FF

#define IR_CLEAR_CAP_2    1 << 6
#define CCR_INT_CAP_2     3 << 7
#define MCR_RESET_MAT_2   1 << 7
#define EMR_EN_MAT_2      1 << 2
#define INITIAL_TIME      0x3FFFFFFF
#define MR_2              2 // match register 2
#define IRQ_MR_2          1 << 6 // enabling irq on match reg 2
#define TOGGLE_MR_2       3 << 8 // enable toggle on match reg 2
#define CLR_FLG_MR_2      1 << 2 // clears match reg 2 active flag
#define CLR_MR_2          7 << 6// clears everything about match reg 2
#define CLR_TOGGLE_MR_2   7 << 7// clears toggle on MR 2
#define IRQ_CAP_2         1<< 8 // interrupt on capture 2
#define REG_CAP_2         3 << 6 // registers for rising, falling, and event on cap 2
#define CR_2              2 // capture register 2
#define MR_1              1 // match register 1
#define IRQ_MR_1          1 << 3 //IRQ on match reg 1
#define IR_MR_1           0x02 // interrupt register on match reg 1
#define CLR_MR_1          7 << 3 // clear match reg 1

#define TEST_TIMER_NUM  0   /* 0 or 1 for 32-bit timers only */
#define TXBUFF_LEN      16
#define RXBUFF_LEN      16
#define BIT_LENGTH  11250 // assuming system clock of 108mHz//5000
//1250115
// baud rates: 9600, 19200, 38400, 57600, 115200
// bit length: 11250, 5625, 2812.5, 1875, 937.5
#define STOP_BIT_SAMPLE (9*BIT_LENGTH)

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

/*********************************************************
** Local Functions                                      **
*********************************************************/
static void swu_setup_tx(void); //tx param processing
 void swu_rx_callback(void); //__weak

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
** parameters:            None
** Returned value:  None
**
******************************************************************************/
static void swu_setup_tx(void) {
  unsigned char bit, i;
  unsigned long int ext_data, delta_edges, mask, reference;
//  hw_digital_write(A_G1, 0);
//  LPC_GPIO_PORT->CLR[4] = (1<<9);
//  GPIOSetValue(PORT_TX_PRO, PIN_TX_PRO, 0); //indicate routine begin
  if (tx_fifo_wr_ind != tx_fifo_rd_ind) //data to send, proceed
  {
    swu_status |= TX_ACTIVE; //sw uart tx is active
    tx_fifo_rd_ind++; //update the tx fifo ...
    if (tx_fifo_rd_ind == TXBUFF_LEN) //read index...
      tx_fifo_rd_ind = 0; //...
    ext_data = (unsigned long int) swu_tx_fifo[tx_fifo_rd_ind]; //read the data
    ext_data = 0xFFFFFE00 | (ext_data << 1); //prepare the pattern
    edge[0] = BIT_LENGTH; //at least 1 falling edge...
    cnt_edges = 1; //... because of the START bit
    bit = 1; //set the bit counter
    reference = 0x00000000; //init ref is 0 (start bit)
    mask = 1 << 1; //prepare the mask
    delta_edges = BIT_LENGTH; //next edge at least 1 bit away
    while (bit != 10) { //until all bits are examined
      if ((ext_data & mask) == (reference & mask)) { //bit equal to the reference?
        delta_edges += BIT_LENGTH; //bits identical=>update length
      } //...
      else { //bits are not the same:
        edge[cnt_edges] = //store new...
          edge[cnt_edges - 1] + delta_edges; //... edge data
        reference = ~reference; //update the reference
        delta_edges = BIT_LENGTH; //reset delta_ to 1 bit only
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
    reference = LPC_TIMER1->TC + BIT_LENGTH; //get the reference from TIMER0
    for (i = 0; i != cnt_edges; i++) //recalculate toggle points...
      edge[i] = (edge[i] + reference) //... an adjust for the...
        & INITIAL_TIME; //... timer range
    LPC_TIMER1->MR[MR_2] = edge[0]; //load MR2
    LPC_TIMER1->MCR = LPC_TIMER1->MCR | (IRQ_MR_2); //enable interrupt on MR2 match
    LPC_TIMER1->EMR = LPC_TIMER1->EMR | (TOGGLE_MR_2); //enable toggle on MR2 match
//    TM_DEBUG("reference %ul", reference);
//    for (i = 0; i < cnt_edges; i++){
//            TM_DEBUG("edge, %ul", edge[i]);
//    }

//  } else {
//          TM_DEBUG("no edge");
  }
//  GPIOSetValue(PORT_TX_PRO, PIN_TX_PRO, 1); //indicate routine exit
//  hw_digital_write(A_G1, 1);
//  LPC_GPIO_PORT->SET[4] = (1<<9);
  return; //return from the routine
}

/******************************************************************************
** Function name:                swu_tx_chr
**
** Descriptions:
**
** This routine puts a single character into the software UART tx FIFO.
**
** parameters:                char
** Returned value:                None
**
******************************************************************************/
void swu_tx_chr(const unsigned char out_char) {
//        hw_digital_write(A_G2, 0);
//        LPC_GPIO_PORT->CLR[5] = (1<<20);
//  GPIOSetValue(PORT_CALL, PIN_CALL, 0); //IOCLR0  = pin_call;
  //write access to tx FIFO begin

  while (swu_tx_cnt == TXBUFF_LEN)
    ; //wait if the tx FIFO is full
  tx_fifo_wr_ind++; //update the write pointer...
  if (tx_fifo_wr_ind == TXBUFF_LEN) //...
    tx_fifo_wr_ind = 0; //...
  swu_tx_fifo[tx_fifo_wr_ind] = out_char; //put the char into the FIFO
  swu_tx_cnt++; //update no.of chrs in the FIFO
  if ((swu_status & TX_ACTIVE) == 0)
    swu_setup_tx(); //start tx if tx is not active

//  GPIOSetValue(PORT_CALL, PIN_CALL, 1); //IOSET0  = pin_call;
//  hw_digital_write(A_G2, 1);
//  LPC_GPIO_PORT->SET[5] = (1<<20);
  //write access to tx FIFO end
  return; //return from the routine
}



/******************************************************************************
** Function name:                swu_tx_str
**
** Descriptions:
**
** This routine transfers a string of characters one by one into the
** software UART tx FIFO.
**
** parameters:                string pointer
** Returned value:                None
**
******************************************************************************/
void swu_tx_str(unsigned char const* ptr_out, uint32_t data_size) {
  while(data_size > 0){
    swu_tx_chr(*ptr_out); //...put the char in tx FIFO...
        ptr_out++; //...move to the next char...
        data_size--;
  }
//  while (*ptr_out != 0x00) //read all chars...
//  {
//    swu_tx_chr(*ptr_out); //...put the char in tx FIFO...
//    ptr_out++; //...move to the next char...
//  } //...
  return; //return from the routine
}

/******************************************************************************
** Function name:                swu_rx_chr
**
** Descriptions:
**
** This routine reads a single character from the software UART rx FIFO.
** If no new data is available, it returns the last one read;
** The framing error indicator is updated, too.
**
** parameters:                None
** Returned value:                char
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
  else
    //... indicator...
    swu_rx_chr_fe = 1; //...
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
void swu_isr_tx(LPC_TIMERn_Type* const TX_ISR_TIMER) {
//        if (edge_index >= 3) {
//                uint32_t test_index_val = edge[edge_index];
//          }
//  if (0 != (TX_ISR_TIMER->IR & (1<<2))) { //tx routine interrupt begin
//    GPIOSetValue(PORT_INT_TX, PIN_INT_TX, 0); //tx interrupt activity begin
//          hw_digital_write(A_G3, 0);
//          LPC_GPIO_PORT->CLR[3] = (1<<8);
    TX_ISR_TIMER->IR = CLR_FLG_MR_2; //0x08; //clear the MAT2 flag
    if (edge_index == char_end_index) //the end of the char:
    {
      TX_ISR_TIMER->MCR &= ~(CLR_MR_2); //MR2 impacts T0 ints no more
      swu_tx_cnt--; //update no.of chars in tx FIFO
      if (tx_fifo_wr_ind != tx_fifo_rd_ind) //if more data pending...
        swu_setup_tx(); //... spin another transmission
      else
        swu_status &= ~TX_ACTIVE; //no data left=>turn off the tx
    } else {
      if (edge_index == last_edge_index) //is this the last toggle?
        TX_ISR_TIMER->EMR &= ~(CLR_TOGGLE_MR_2); //0x000003FF; //no more toggle on MAT2
      edge_index++; //update the edge index
      TX_ISR_TIMER->MR[MR_2] = edge[edge_index]; //prepare the next toggle event
//      if (edge_index >= 3) {
//              uint32_t index_val= edge[edge_index];
//      }
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
void swu_isr_rx(LPC_TIMERn_Type* const RX_ISR_TIMER) {


  signed long int edge_temp;
  //sw uart receive isr code begin
  if (0 != (RX_ISR_TIMER->IR & (IR_CLEAR_CAP_2))) //capture interrupt occured on CAP2
  {
//    GPIOSetDir(PORT_INT_RX, PIN_INT_RX, 0); //rx interrupt activity begin
    LPC_GPIO_PORT->SET[1] = (1 << 8); // C_G2 high
    LPC_GPIO_PORT->DIR[3] |= (1 << 10);
    RX_ISR_TIMER->IR = IR_CLEAR_CAP_2;//0x10; //edge detcted=>clear CAP2 flag
    //change the targeted edge
    RX_ISR_TIMER->CCR = (IRQ_CAP_2) | ((REG_CAP_2) - (RX_ISR_TIMER->CCR & (REG_CAP_2)));//0x0004 | (0x0003 - (RX_ISR_TIMER->CCR & 0x0003));
    if ((swu_status & RX_ACTIVE) == 0) { //sw UART not active (start):
      edge_last = (signed long int) RX_ISR_TIMER->CR[CR_2]; //initialize the last edge
      edge_sample = edge_last + (BIT_LENGTH >> 1); //initialize the sample edge
      if (edge_sample < edge_last) //adjust the sample edge...
        edge_sample |= ADJUST; //... if needed
      swu_bit = 0; //rx bit is 0 (a start bit)
      // use mat1 to match stop bit timing
      RX_ISR_TIMER->IR = 0x01; //clear MAT1 int flag
      edge_stop = edge_sample + STOP_BIT_SAMPLE; //estimate the end of the byte
      if (edge_stop < edge_last) //adjust the end of byte...
        edge_stop |= ADJUST; //... if needed
      RX_ISR_TIMER->MR[MR_1] = edge_stop; //set MR1 (stop bit center)
      RX_ISR_TIMER->MCR = RX_ISR_TIMER->MCR | (IRQ_MR_1); //int on MR1
      cnt = 9; //initialize the bit counter
      swu_status |= RX_ACTIVE; //update the swu status
      swu_rbr = 0x0000; //reset the sw rbr
      swu_rbr_mask = 0x0001; //initialize the mask
    } else {
      //reception in progress:
      edge_current = (signed long int) RX_ISR_TIMER->CR[CR_2]; //initialize the current edge
      if (edge_current < edge_last) //adjust the current edge...
        edge_current |= ADJUST; //... if needed
      while (edge_current > edge_sample) { //while sampling edge is within
        if (cnt_bits != 0) {
          if (swu_bit != 0) //update data...
            swu_rbr |= swu_rbr_mask; //...
          swu_rbr_mask = swu_rbr_mask << 1; //update mask
        }
        cnt_bits++; //update the bit count
        edge_temp = edge_last + BIT_LENGTH; //estimate the last edge
        if (edge_temp < edge_last) //adjust...
          edge_last = edge_temp | ADJUST; //... the last edge...
        else
          //... if...
          edge_last = edge_temp; //... needed
        edge_temp = edge_sample + BIT_LENGTH; //estimate the sample edge
        if (edge_temp < edge_sample) //adjust...
          edge_sample = edge_temp | ADJUST; //... the sample edge...
        else
          //... if...
          edge_sample = edge_temp; //... needed
        cnt--; //update the no of rcved bits
      }
      swu_bit = 1 - swu_bit; //change the received bit
    }
    LPC_GPIO_PORT->CLR[1] = (1 << 8); // C_G2 low
//    GPIOSetDir(PORT_INT_RX, PIN_INT_RX, 1); //rx interrupt activity end
    LPC_GPIO_PORT->DIR[3] &= ~(1 << 10);
  }

  if (0 != (RX_ISR_TIMER->IR & IR_MR_1)) //stop bit timing matched:
  {
//    GPIOSetDir(PORT_INT_RX, PIN_INT_RX, 0); //rx interrupt activity begin
    LPC_GPIO_PORT->DIR[3] |= (1 << 10);
//          hw_digital_write(C_G1, 0);
    LPC_GPIO_PORT->SET[0] = (1 << 11); // C_G1 high
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
    RX_ISR_TIMER->MCR &= ~(CLR_MR_1);//MR1 impacts timer1 no more
//    hw_digital_write(C_G1, 1);
    LPC_GPIO_PORT->CLR[0] = (1 << 11); // clear C_G1
//    GPIOSetDir(PORT_INT_RX, PIN_INT_RX, 1); //IOSET0 = pin_intrx;     //rx interrupt activity end
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
uint8_t volatile timer0_flag = FALSE, timer1_flag = FALSE;

void TIMER1_IRQHandler(void)
{
  // flag for cap2 and mat1
//  if ((LPC_TIMER1->IR & (1<<6)) || (LPC_TIMER1->IR & (1<<1)) ) {
//    LPC_GPIO_PORT->CLR[4] = (1<<15); // C_G3 low
//    LPC_GPIO_PORT->SET[4] = (1<<15); // C_G3 high
//
//    swu_isr_rx(LPC_TIMER1);
//  }

  //tx stuff
        if (0 != (LPC_TIMER1->IR & (1<<2)))  //tx routine interrupt begin
        {
//                LPC_GPIO_PORT->CLR[1] = (1<<8);
                swu_isr_tx(LPC_TIMER1);
//                LPC_GPIO_PORT->SET[1] = (1<<8);

//          if (edge_index >=2) {
//                  uint32_t LPCTCR = LPC_TIMER1->TCR;
//                  uint32_t LPCMCR = LPC_TIMER1->MCR;
//                  uint32_t LPCMR0 = LPC_TIMER1->MR[0];
//                  uint32_t LPCMR1 = LPC_TIMER1->MR[1];
//                  uint32_t LPCMR2 = LPC_TIMER1->MR[2];
//                  uint32_t LPCMR3 = LPC_TIMER1->MR[3];
//                  uint32_t LPCEMR = LPC_TIMER1->EMR;
//                  uint32_t LPCIR = LPC_TIMER1->IR;
//                  uint32_t LPCPR = LPC_TIMER1->PR;
//                  uint32_t index_val= edge[edge_index];
//          }
        }

        //                hw_digital_write(C_G2, 1);
}

void swu_rx_callback(void)
{
  //append flag bit to character
  rxData =0x100 + swu_rx_chr();
  return;
}
/******************************************************************************
** Function name:         swu_init
**
** Description:                Initialize variables used by software UART.
**                                         Configure specific IO pins.
**                                         Setup and start timer running.
**
** parameters:          UART_TIMER  - pointer to timer resource to enable configuration.
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

  hw_digital_write(A_G1, 1);
  hw_digital_write(A_G2, 1);
  hw_digital_write(A_G3, 1);
  hw_digital_write(C_G1, 1);
  hw_digital_write(C_G2, 1);
  hw_digital_write(C_G3, 1);
  scu_pinmux(0x0B ,1 , MD_PDN, FUNC5);   // CTOUT_6  SCT output 6. Match output 2 of timer 1.
  scu_pinmux(7 ,2 , MD_PLN_FAST, FUNC1); // CTIN_4 „ SCT input 4. Capture input 2 of timer 1.

  // Initialize timer 1, prescale count time of 100uS
//  CGU_ConfigPWR(CGU_PERIPHERAL_TIMER1, ENABLE);
//  uint32_t timer0_freq = CGU_GetPCLKFrequency(CGU_PERIPHERAL_TIMER1);

  LPC_TIMER1->TCR = 0x00;
  LPC_TIMER1->TCR = 0x02; //reset counter
  LPC_TIMER1->TCR = 0x00; //release the reset

  LPC_TIMER1->IR = 0x0FF; //clear all TIMER0 flags
  LPC_TIMER1->PR = 0x00000000; //no prescaler
  LPC_TIMER1->MR[0] = INITIAL_TIME; //TIMER0_32 counts 0 - 0x3FFFFFFF
  LPC_TIMER1->MCR = MR_2;//MCR_RESET_MAT_2; //1<<7; //reset TIMER1 on MR2
  // MCR =  3 << 6; do the BIT_LENGTH baud with match reg 2
  LPC_TIMER1->EMR = EMR_EN_MAT_2; //1<<2; //enable timer 0 match 2

//  while (0 == (LPC_GPIO_PORT->PIN[0x07] & (1 << 2)))
//          ;//wait for 1 on sw UART rx line
  hw_digital_write(C_G2, 1);
  hw_digital_write(C_G2, 0);

  LPC_TIMER1->IR = IR_CLEAR_CAP_2;//1 << 6; // //clear CAP1.2 flag
  LPC_TIMER1->CCR = CCR_INT_CAP_2; //3 << 7; //int on CAP1.2 falling edge

  LPC_TIMER1->TCR = 0x01; //let TIMER0_32 run
  NVIC_EnableIRQ(TIMER1_IRQn); //Enable the TIMER0_32 Interrupt

  cnt_bits = 0; //reset the rx bit count
}

void TIMER0_IRQHandler(void)
{
//        if (TIM_GetIntStatus(LPC_TIMER0, TIM_MR0_INT)== SET)
//        {
//                LPC_GPIO_PORT->CLR[1] = (1<<8);

//                LPC_GPIO_PORT->SET[1] = (1<<8);
    hw_digital_write(B_G1, 1);
    hw_digital_write(B_G1, 0);
//        }
//        TIM_ClearIntPending(LPC_TIMER0, TIM_MR0_INT);
}

void timer_test(void) {
  // Initialize timer 0, prescale count time of 100uS
  TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_TICKVAL;//TIM_PRESCALE_USVAL;
  TIM_ConfigStruct.PrescaleValue        = 1;//100;

  // use channel 0, MR0
  TIM_MatchConfigStruct.MatchChannel = 0;
  // Enable interrupt when MR0 matches the value in TC register
  TIM_MatchConfigStruct.IntOnMatch   = TRUE;
  //Enable reset on MR0: TIMER will reset if MR0 matches it
  TIM_MatchConfigStruct.ResetOnMatch = TRUE;
  //Stop on MR0 if MR0 matches it
  TIM_MatchConfigStruct.StopOnMatch  = FALSE;
  //Toggle MR0.0 pin if MR0 matches it
  TIM_MatchConfigStruct.ExtMatchOutputType =TIM_EXTMATCH_TOGGLE;
  // Set Match value, count value of 10000 (10000 * 100uS = 1000000us = 1s --> 1 Hz)
  TIM_MatchConfigStruct.MatchValue   = BIT_LENGTH;

  // Set configuration for Tim_config and Tim_MatchConfig
  TIM_Init(LPC_TIMER0, TIM_TIMER_MODE,&TIM_ConfigStruct);
  TIM_ConfigMatch(LPC_TIMER0,&TIM_MatchConfigStruct);

  // preemption = 1, sub-priority = 1
  NVIC_SetPriority(TIMER0_IRQn, ((0x01<<3)|0x01));
  // Enable interrupt for timer 0
  NVIC_EnableIRQ(TIMER0_IRQn);
  // To start timer 0
  TIM_Cmd(LPC_TIMER0,ENABLE);
}

void capture_test(void){
  hw_digital_write(C_G3, 1);
  scu_pinmux(0x7,2,MD_PLN_FAST, FUNC1);

  // Initialize timer 0, prescale count time of 1000000uS = 1S
  TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_TICKVAL;//TIM_PRESCALE_USVAL;
  TIM_ConfigStruct.PrescaleValue  = 1;//100;

  // use channel 0, CAPn.CAPTURE_CHANNEL
  TIM_CaptureConfigStruct.CaptureChannel = 2;
  // Enable capture on CAPn.CAPTURE_CHANNEL rising edge
//  TIM_CaptureConfigStruct.RisingEdge = ENABLE;
  // Enable capture on CAPn.CAPTURE_CHANNEL falling edge
  TIM_CaptureConfigStruct.FallingEdge = ENABLE;
  // Generate capture interrupt
  TIM_CaptureConfigStruct.IntOnCaption = ENABLE;


  // Set configuration for Tim_config and Tim_MatchConfig
  TIM_Init(LPC_TIMER0, TIM_TIMER_MODE,&TIM_ConfigStruct);
  TIM_ConfigCapture(LPC_TIMER1, &TIM_CaptureConfigStruct);
  TIM_ResetCounter(LPC_TIMER1);


  /* preemption = 1, sub-priority = 1 */
  NVIC_SetPriority(TIMER1_IRQn, ((0x01<<3)|0x01));
  /* Enable interrupt for timer 0 */
  NVIC_EnableIRQ(TIMER1_IRQn);
  // To start timer 0
  TIM_Cmd(LPC_TIMER1,ENABLE);
}

void sw_uart_test_c(void) {
//timer_test();
//  capture_test();
  swu_init();

  // tx test
  uint8_t data[] = {0x56, 0x00, 0x11, 0x00};
  swu_tx_str(data, sizeof(data));

  // rx test
//  int numInBuff = 0;
//  char * buff[10];
//  while (1){
//    if (rxData & 0x100 && numInBuff < 10)//valid flag indicating new character
//      {
//        LPC_GPIO_PORT->SET[5] = (1<<25); // b_g3
//        LPC_GPIO_PORT->CLR[5] = (1<<25); // b_g3
//        unsigned char buffChar = (unsigned char) rxData & 0xFF;
//        buff[numInBuff] = buffChar;
//        rxData=0;
//        numInBuff++;
//      }
//    else if (numInBuff >= 10){
//      for (int i = 0; i< numInBuff; i ++){
//        TM_DEBUG("buffer has %c", buff[i]);
//      }
//    }
//  };
//  NVIC_DisableIRQ(TIMER1_IRQn); //Disable the timer Interrupt
}
