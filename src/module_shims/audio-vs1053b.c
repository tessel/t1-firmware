#include "audio-vs1053b.h"
#include "string.h"

void audio_spi_callback();
void audio_interrupt_callback();

uint8_t *tx_buf;
uint8_t *rx_buf;

int audio_play_buffer(uint8_t cs, uint8_t dreq, const uint8_t *buf, uint32_t buf_len) {
  cs++;
  dreq++;
  buf++;
  buf_len++;  

  // Initialize SPI (I guess we can do this outside of C, in JS)
  spi_async_status.callback = &audio_spi_callback;

  tx_buf = malloc(20);
  rx_buf = malloc(20);
  char data[] = "Sending Data";
  memcpy(tx_buf, data, sizeof(data));

  TM_DEBUG("Beginning transfer...");
  // hw_spi_transfer(0, 20, 20, tx_buf, rx_buf, -2, -2);

  uint8_t assignment = hw_interrupt_acquire();
  TM_DEBUG("assignment %d", assignment);
  hw_interrupt_watch(dreq, 2, assignment, audio_interrupt_callback);

  // while there are still bytes to transfer
    // wait for dreq to go low
      // Pull CS low
      // transfer 32 bytes
        // wait for transfer to complete.
          // Pull CS high
  return 17;
}

void audio_spi_callback() {
  TM_DEBUG("Callback was hit!");
  free(tx_buf);
  free(rx_buf);
}

void audio_interrupt_callback() {
  TM_DEBUG("In the interrupt callback!");
}