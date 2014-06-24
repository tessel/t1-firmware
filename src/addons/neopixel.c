#include "neopixel.h" 

// Our CPU Frequency is 180MHz
#define CPU_FREQUENCY 180000000.0
// The frequency of these signals is 800KHz
#define NEO_FREQUENCY 800000.0
// The period of a signal is the inverse of the frequency
#define NEO_PERIOD (float)(CPU_FREQUENCY / NEO_FREQUENCY)
// The width of the pull is the duration of the width in uS
#define NEO_FULL_WIDTH (float)(1000000.0 / NEO_FREQUENCY)
// A one duty cycle is the ratio of how long a signal is high in a period
#define NEO_ONE_DUTY_CYCLE (float)(0.85 /NEO_FULL_WIDTH)
// A zero duty cycle is the ratio of how long a signal is high in a period
#define NEO_ZERO_DUTY_CYCLE (float)(0.4 / NEO_FULL_WIDTH) 
// A one pulse width duration
#define NEO_ONE_PULSE_WIDTH (float)(NEO_ONE_DUTY_CYCLE * NEO_PERIOD)
// A zero pulse width duration
#define NEO_ZERO_PULSE_WIDTH (float)(NEO_ZERO_DUTY_CYCLE * NEO_PERIOD)

// The actual DMA we'll be using to transfer data
// Async SPI uses 0 and 1 so we don't want to mess with that
#define SCT_DMA_CHAN_0 2
#define SCT_DMA_CHAN_1 3
#define SCT_DMA_CHAN_2 4

// The address of the first match register
#define MATCH_0_ADDRESS 0x40000100

const uint8_t buff[6] = { NEO_ONE_PULSE_WIDTH, NEO_ONE_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH, NEO_ONE_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH} ;

void basicTest() {

  uint8_t pin = E_G4;

  // GPDMA channel configuration struct
  // Contains information about source, destination, and transferType
  hw_GPDMA_Chan_Config chan_1;

  // Set the destination to be SCT channel 0
  chan_1.DestConn = GPDMA_CONN_SCT_0;
  // Set the source connection to zero since it will be our buffer
  chan_1.SrcConn = 0;
  // Set the transfer type to memory->peripheral (SCT)
  chan_1.TransferType = m2p;

  // Config the DMAMUX register for this kind of transfer
  hw_gpdma_transfer_config(SCT_DMA_CHAN_0, &chan_1);

  // Create the memory for the dma linked list
  hw_GPDMA_Linked_List_Type * Linked_List = calloc(1, sizeof(hw_GPDMA_Linked_List_Type));

  // Set the source to our buffer
  Linked_List[0].Source = (uint32_t) buff;
  // Set the detination to the address of the SCT Connection
  Linked_List[0].Destination = (uint32_t) MATCH_0_ADDRESS;//LPC_GPIO_PORT->PIN + g_APinDescription[pin].port;
  // Use dest AHB Master 1 and source increment. Transfer 1 byte (which is actually the duty cycle for one pixel bit)
  Linked_List[0].Control = ((1UL<<25) | (1UL<<26) | 1);
  // Set this as the end of linked list
  Linked_List[0].NextLLI = (uint32_t)0;

  // Tell SCT That we want event 0 (period match) to trigger DMA request 0
  LPC_SCT->DMA0REQUEST = (1 << 0);

  TM_DEBUG("Starting DMA transfer with a pulse width of %.6f for one and %.6f for zero\n", NEO_ONE_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH);
  // Enable DMA transfers (in ../hw/gpdma.c)
  hw_gpdma_transfer_begin(SCT_DMA_CHAN_0, Linked_List);
  TM_DEBUG("Transfer began");
  TM_DEBUG("What? %d %d", LPC_GPIO_PORT->PIN, g_APinDescription[pin].port);
  // Start PWM (in ../hw/pwm.c)
  hw_pwm_enable(pin, PWM_EDGE_HI, (uint32_t)NEO_PERIOD, (uint32_t)NEO_ZERO_PULSE_WIDTH);

  TM_DEBUG("Sent!");
}