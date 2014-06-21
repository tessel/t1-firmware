#include "neopixel.h" 

// Our CPU Frequency is 180MHz
#define CPU_FREQUENCY 180000000
// The frequency of these signals is 800KHz
#define NEO_FREQUENCY 800000
// The period of a signal is the inverse of the frequency
#define NEO_PERIOD (CPU_FREQUENCY / NEO_FREQUENCY)
// The width of the pull is the duration of the width in uS
#define NEO_FULL_WIDTH 1000000 / NEO_FREQUENCY
// A one duty cycle is the ratio of how long a signal is high in a period
#define NEO_ONE_DUTY_CYCLE (0.85 / NEO_FULL_WIDTH)
// A zero duty cycle is the ratio of how long a signal is high in a period
#define NEO_ZERO_DUTY_CYCLE (0.4 / NEO_FULL_WIDTH) 
// A one pulse width duration
#define NEO_ONE_PULSE_WIDTH NEO_ONE_DUTY_CYCLE * NEO_PERIOD
// A zero pulse width duration
#define NEO_ZERO_PULSE_WIDTH NEO_ZERO_DUTY_CYCLE * NEO_PERIOD

// The actual DMA we'll be using to transfer data
// Async SPI uses 0 and 1 so we don't want to mess with that
#define SCT_DMA_CHAN_1 2
#define SCT_DMA_CHAN_2 3
#define SCT_DMA_CHAN_3 4

const uint8_t buff[6] = { NEO_ONE_PULSE_WIDTH, NEO_ONE_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH, NEO_ONE_PULSE_WIDTH, NEO_ZERO_PULSE_WIDTH} ;

void basicTest() {

  hw_GPDMA_Chan_Config chan_1;

  // Set the destination to be SCT channel 0
  chan_1.DestConn = GPDMA_CONN_SCT_0;
  // Set the source connection to zero since it will be our buffer
  chan_1.SrcConn = 0;
  // Set the transfer type to memory->peripheral (SCT)
  chan_1.TransferType = m2p;

  // Config the DMAMUX register for this kind of transfer
  hw_gpdma_transfer_config(SCT_DMA_CHAN_1, &chan_1);

  // Create the memory for the dma linked list
  hw_GPDMA_Linked_List_Type * Linked_List = calloc(1, sizeof(hw_GPDMA_Linked_List_Type));

  // Set the source to our buffer
  Linked_List[0].Source = (uint32_t) buff;
  // Set the detination to the address of the SCT Connection
  Linked_List[0].Destination = (uint32_t) hw_gpdma_get_lli_conn_address(chan_1.DestConn);
  // Se tthe control to read 4 byte bursts with max buf len, and increment the destination counter
  Linked_List[0].Control = ((1UL<<24) | (1UL<<27) | sizeof(buff));
  // Set this as the end of linked list
  Linked_List[0].NextLLI = (uint32_t)0;

  // Get a buffer of values

  // Set up DMA

  // Send the buffer to SCT
}