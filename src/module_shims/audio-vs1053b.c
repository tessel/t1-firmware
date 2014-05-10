#include "audio-vs1053b.h"

void audio_play_buffer(uint8_t cs, uint8_t dreq, uint8_t buf) {
  cs++;
  dreq++;
  buf++;

  // Initialize SPI

  // while there are still bytes to transfer
    // wait for dreq to go low
      // Pull CS low
      // transfer 32 bytes
        // wait for transfer to complete.
          // Pull CS high
}