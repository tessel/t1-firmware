#include "hw.h"
#include "tm.h"
#include "colony.h"

void async_spi_callback();
/// The event triggered by the timer callback
tm_event async_spi_event = TM_EVENT_INIT(async_spi_callback);

volatile struct spi_async_status_t spi_async_status = {
  .txRef = LUA_NOREF,
  .rxRef = LUA_NOREF,
  .txLength = 0,
  .rxLength = 0,
  .tx_Linked_List = 0,
  .rx_Linked_List = 0,
};

void hw_spi_async_status_reset () {
  spi_async_status.tx_Linked_List = 0;
  spi_async_status.rx_Linked_List = 0;
  spi_async_status.txLength = 0;
  spi_async_status.rxLength = 0;
  spi_async_status.txRef = LUA_NOREF;
  spi_async_status.rxRef = LUA_NOREF;
}

void hw_spi_dma_counter (uint8_t channel){
  // Check counter terminal status
  if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, channel)){
    // Clear terminate counter Interrupt pending
    GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, channel);
    // This interrupt status should be hit for both tx and rx
    uint8_t done_count = (spi_async_status.txLength && spi_async_status.rxLength) ? 2 : 1;
    if (++spi_async_status.transferCount == done_count) {
      // Trigger the callback
      tm_event_trigger(&async_spi_event);
    }
  }
  if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, channel)){
    // Clear error counter Interrupt pending
    GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, channel);
    // Register the error in our struct
    spi_async_status.transferError++;
    // Trigger the callback because there was an error
    tm_event_trigger(&async_spi_event);
  }
}

hw_GPDMA_Linked_List_Type * hw_spi_dma_packetize (size_t buf_len, uint32_t source, uint32_t destination, uint8_t txBool) {

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
    base_control =  ((1UL<<25)) | ((1UL<<26));
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


void hw_spi_async_cancel_transfers () {
  hw_gpdma_cancel_transfer(0);
  hw_gpdma_cancel_transfer(1);
}

void hw_spi_async_cleanup () {
  // Unreference our buffers so they can be garbage collected
  luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, spi_async_status.txRef);
  luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, spi_async_status.rxRef);
  
  // Free our linked list 
  free(spi_async_status.tx_Linked_List);
  free(spi_async_status.rx_Linked_List);

  // Clear our config struct
  hw_spi_async_status_reset();

  // If there are any current transfers, stop them
  hw_spi_async_cancel_transfers();

  // Unreference the event to free up the event queue
  tm_event_unref(&async_spi_event);
}

void async_spi_callback (void) {
  // Make sure the Lua state exists
  lua_State* L = tm_lua_state;
  if (!L) return;

  // Push the _colony_emit helper function onto the stack
  lua_getglobal(L, "_colony_emit");
  // The process message identifier
  lua_pushstring(L, "spi_async_complete");
  // push whether we got an error (1 or 0)
  lua_pushnumber(L, spi_async_status.transferError);
  // Clean up our vars so that we can do this again
  hw_spi_async_cleanup();
  // Call _colony_emit to run the JS callback
  tm_checked_call(L, 2);
}

int hw_spi_transfer (size_t port, size_t txlen, size_t rxlen, const uint8_t *txbuf, const uint8_t *rxbuf, uint32_t txref, uint32_t rxref)
{
  hw_spi_t *SPIx = find_spi(port);

  uint8_t tx_chan = 0;
  uint8_t rx_chan = 1;
  
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

  hw_spi_async_status_reset();

  if (txlen != 0) {
     // Source Connection - unused
    tx_config.SrcConn = 0;
    // Transfer type
    tx_config.TransferType = m2p;

    // Configure the tx transfer on channel 0
    // TODO: Get next available channel
    hw_gpdma_transfer_config(tx_chan, &tx_config);

    // Save the length that we're transferring
    spi_async_status.txLength = txlen;

    // Generate the linked list structure for transmission
    spi_async_status.tx_Linked_List = hw_spi_dma_packetize(txlen, (uint32_t)txbuf, hw_gpdma_get_lli_conn_address(tx_config.DestConn), 1);

    // Begin the transmission
    hw_gpdma_transfer_begin(tx_chan, spi_async_status.tx_Linked_List);
  }

  if (rxlen != 0) {
    // Destination connection - unused
    rx_config.DestConn = 0;
    // Transfer type
    rx_config.TransferType = p2m;

    // Configure the rx transfer on channel 1
    // TODO: Get next available channel
    hw_gpdma_transfer_config(rx_chan, &rx_config);

    // Save the length that we're transferring
    spi_async_status.rxLength = rxlen;

    // Generate the linked list structure for receiving
    spi_async_status.rx_Linked_List = hw_spi_dma_packetize(rxlen, hw_gpdma_get_lli_conn_address(rx_config.SrcConn), (uint32_t)rxbuf, 0);

    // Begin the reception
    hw_gpdma_transfer_begin(rx_chan, spi_async_status.rx_Linked_List);
  }

  spi_async_status.txRef = txref;
  spi_async_status.rxRef = rxref;
  spi_async_status.transferCount = 0;
  spi_async_status.transferError = 0;


  // if it's a slave pull down CS
  if (SPIx->is_slave) {
    scu_pinmux(0xF, 1u, PUP_DISABLE | PDN_ENABLE |  MD_EZI | MD_ZI | MD_EHS, FUNC2);  //  SSP0 SSEL0
  }

  // Tell the runtime to keep the event loop active until this event is done
  tm_event_ref(&async_spi_event);

  return 0;
}
