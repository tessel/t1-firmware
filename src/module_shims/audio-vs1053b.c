#include "audio-vs1053b.h"

int audio_play_buffer(uint8_t cs, uint8_t dreq, const uint8_t *buf, uint32_t buf_len) {
  cs++;
  dreq++;
  buf++;
  buf_len++;  

  // Initialize SPI (I guess we can do this outside of C, in JS)

  // while there are still bytes to transfer
    // wait for dreq to go low
      // Pull CS low
      // transfer 32 bytes
        // wait for transfer to complete.
          // Pull CS high
  return 17;
}