#include "audio-vs1053b.h"

#define AUDIO_CHUNK_SIZE 32
#define SPI_PORT 0

typedef struct AudioBuffer{
  struct AudioBuffer *next;
  uint8_t *tx_buf;
  uint8_t remaining_bytes;
  uint8_t chip_select;
  uint8_t dreq;
  uint8_t interrupt;
  uint16_t stream_id;
} AudioBuffer;

// Callbacks for SPI and GPIO Interrupt
void _audio_spi_callback();
void _audio_interrupt_callback();

// Linked List queueing methods
void _queue_buffer(AudioBuffer *buffer);
void _shift_buffer();

// Method to start listening for DREQ pulled low
void _audio_begin_play();
void _audio_emit_completion(uint16_t stream_id);
uint16_t _generate_stream_id();

AudioBuffer *operating_buf = NULL;

uint16_t master_id_gen = 0;

void _queue_buffer(AudioBuffer *buffer) {

  AudioBuffer **next = &operating_buf;
  // If we don't have an operating buffer already
  if (*next == NULL) {
    // Assign this buffer to the head
    *next = buffer;

    // Start watching for a low DREQ
    _audio_begin_play();
  }

  // If we do already have an operating buf
  else {
    // Iterate to the last buf
    while (*next) { next = &(*next)->next; }

    // Set this one as the next
    *next = buffer;
  }
}

void _shift_buffer() {
    
  AudioBuffer *head = operating_buf;

  // If the head is already null
  if (head == NULL) {
    // Just return
    return;
  }
  else {
    // Save the ref to the head
    AudioBuffer *old = head;

    // Set the new head to the next
    operating_buf = old->next;

    // Clean any references to the previous transfer
    free(old->tx_buf);
    free(old);

    // If there is another buffer to play
    if (operating_buf != NULL) {
      TM_DEBUG("New head is not null...");
      // Begin the next transfer
      _audio_begin_play();
    }
    return;
  }
}

int audio_play_buffer(uint8_t chip_select, uint8_t dreq, const uint8_t *buf, uint32_t buf_len) {

  // SPI.initialize MUST be initialized before calling this func

  // Create a new buffer struct
  AudioBuffer *new_buf = malloc(sizeof(AudioBuffer));
  // Set the chip select field
  new_buf->chip_select = chip_select;
  // Set the chip select pin as an output
  hw_digital_output(new_buf->chip_select);
  // Set the dreq field
  new_buf->dreq = dreq;
  // Malloc the space for the txbuf
  new_buf->tx_buf = malloc(buf_len);
  // Copy over the bytes from the provided buffer
  memcpy(new_buf->tx_buf, buf, buf_len);
  // Set the length field
  new_buf->remaining_bytes = buf_len;
  // Set the next field
  new_buf->next = 0;
  // Generate an ID for this stream so we know when it's complete
  new_buf->stream_id = _generate_stream_id();

  _audio_begin_play();

  return new_buf->stream_id;
}

// Called when a SPI transaction has completed
void _audio_spi_callback() {
  TM_DEBUG("Callback was hit!");
  // Pull chip select back high again
  hw_digital_write(operating_buf->chip_select, 1);
  // As long as we have an operating buf
  if (operating_buf != NULL) {
    // Check if we have more bytes to send for this buffer
    if (operating_buf->remaining_bytes > 0) {
      // Watch for when DREQ goes low
      _audio_begin_play();
    }
    // We've completed playing all the bytes for this buffer
    else {
      uint16_t stream_id = operating_buf->stream_id;
      // Remove this buffer from the linked list and free the memory
      _shift_buffer();

      // Emit the event that we finished playing that buffer
      _audio_emit_completion(stream_id);
    }
  }
}

// Called when DREQ goes low. Data is available to be sent over SPI
void _audio_interrupt_callback() {
  TM_DEBUG("In the interrupt callback!");
  // If we have an operating buf
  if (operating_buf != NULL) {
    // Figure out how many bytes we're sending next
    uint8_t to_send = operating_buf->remaining_bytes < AUDIO_CHUNK_SIZE ? operating_buf->remaining_bytes : AUDIO_CHUNK_SIZE;
    // Set the callback to our own function
    spi_async_status.callback = &_audio_spi_callback;
    // Pull chip select low
    hw_digital_write(operating_buf->chip_select, 0);
    // Transfer the data
    hw_spi_transfer(SPI_PORT, to_send, 0, operating_buf->tx_buf, NULL, -2, -2);
    // Update our buffer position
    operating_buf->tx_buf += to_send;
    // Reduce the number of butes remaining
    operating_buf->remaining_bytes -= to_send;
  }
}

void _audio_begin_play() {
  // Start watching dreq for a low signal
  hw_interrupt_watch(operating_buf->dreq, TM_INTERRUPT_MODE_LOW, operating_buf->interrupt, _audio_interrupt_callback);
}

void _audio_emit_completion(uint16_t stream_id) {
  lua_State* L = tm_lua_state;
  if (!L) return;

  lua_getglobal(L, "_colony_emit");
  lua_pushstring(L, "audio_complete");
  lua_pushnumber(L, stream_id);
  tm_checked_call(L, 2);
}

uint16_t _generate_stream_id() {
  return ++master_id_gen;
}