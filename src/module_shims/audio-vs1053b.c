// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "audio-vs1053b.h"

#define DEBUG

// TODO: refactor this
// I'm using the same struct for both playback
// and recording data
typedef struct AudioBuffer{
  struct AudioBuffer *next;
  uint8_t *buffer;
  uint32_t remaining_bytes;
  uint32_t buffer_position;
  uint8_t command_select;
  uint8_t data_select;
  uint8_t dreq;
  uint8_t interrupt;
  uint16_t stream_id;
  int32_t buffer_ref;
} AudioBuffer;

// State definitions
enum State { PLAYING, STOPPED, PAUSED, RECORDING, CANCELLING };

// Callbacks for SPI and GPIO Interrupt
static void audio_continue_spi();

// Linked List queueing methods
static void queue_buffer(AudioBuffer *buffer);
static void shift_buffer();

// Internal Playback methods
static void audio_watch_dreq();
static void audio_emit_completion(uint16_t stream_id);
static uint16_t generate_stream_id();
static void audio_flush_buffer();
static void sendEndFillBytes(uint32_t num_fill);

// Internal Recording methods
static uint16_t loadPlugin(const char *plugin_dir);
// Timer handler for reading data from the recording buffer
void RIT_IRQHandler(void);
static void recording_register_check();
static void startRIT(void);
static void stopRIT(void);
static size_t read_recorded_data(uint8_t *data, size_t len);
static void freeRecordingResources();

// Methods for controlling the device
static void softReset();
static void setGPIOAsOutput();
static void writeSciRegister16(uint8_t address_byte, uint16_t data);
static uint16_t readSciRegister16(uint8_t address_byte);

// Buffer for loading data into before passing into runtime
uint8_t *double_buff = NULL;

AudioBuffer *operating_buf = NULL;
// Generates "unique" values for each pending stream
uint16_t master_id_gen = 0;
// The current state of the mp3 player
enum State current_state = STOPPED;

uint32_t queue_length = 0;

static void queue_buffer(AudioBuffer *buffer) {

  #ifdef DEBUG
  TM_DEBUG("ADD: %d items in the queue", ++queue_length);
  #endif

  AudioBuffer **next = &operating_buf;
  // If we don't have an operating buffer already
  if (!(*next)) {
    // Assign this buffer to the head
    *next = buffer;

    #ifdef DEBUG
    TM_DEBUG("First buffer in queue. Beginning execution immediately."); 
    #endif

    // Start watching for a low DREQ
    audio_watch_dreq();
  }

  // If we do already have an operating buffer
  else {

    #ifdef DEBUG
    TM_DEBUG("Placing at the end of the queue"); 
    #endif
    // Iterate to the last buffer
    while (*next) { next = &(*next)->next; }

    // Set this one as the next
    *next = buffer;
  }
}

static void shift_buffer() {
  #ifdef DEBUG
  TM_DEBUG("SHIFT: %d items in the queue", --queue_length);
  #endif

  // If the head is already null
  if (!operating_buf) {
    // Just return
    return;
  }
  else {
    // Save the stream id for our event emission
    uint16_t stream_id = operating_buf->stream_id;

    if (!operating_buf->next) {
      #ifdef DEBUG
      TM_DEBUG("Final item in the queue complete. Flushing all buffers"); 
      #endif
      audio_stop_buffer(false);
    }
    else {
      #ifdef DEBUG
      TM_DEBUG("Loading next buffer"); 
      #endif
      // Save the ref to the head
      AudioBuffer *old = operating_buf;

      // Set the new head to the next
      operating_buf = old->next;
      // Clean any references to the previous transfer
      free(old->buffer);
      free(old);

      #ifdef DEBUG
      TM_DEBUG("Playing next item in queue"); 
      #endif
      // Begin the next transfer
      audio_watch_dreq();
    }

    // Emit the event that we finished playing that buffer
    audio_emit_completion(stream_id);
  }
}

static void audio_flush_buffer() {

  #ifdef DEBUG
  TM_DEBUG("Flushing buffer."); 
  #endif

  uint16_t mode = readSciRegister16(VS1053_REG_MODE);

  writeSciRegister16(VS1053_REG_MODE, mode | (VS1053_MODE_SM_CANCEL));

  // Free our entire queue 
  for (AudioBuffer **curr = &operating_buf; *curr;) {
    // Save previous as the current linked list item
    AudioBuffer *prev = *curr;

    // Current is now the next item
    *curr = prev->next;

    // Free the previous item
    free(prev);
  }

  master_id_gen = 0;
  operating_buf = NULL;
  queue_length = 0;
}

// Called when DREQ goes low. Data is available to be sent over SPI
static void audio_continue_spi() {
  // If we have an operating buffer
  if (operating_buf != NULL) {

    // While dreq is held high and we have more bytes to send
    while (hw_digital_read(operating_buf->dreq) && operating_buf->remaining_bytes > 0) {
      // Figure out how many bytes we're sending next
      uint8_t to_send = operating_buf->remaining_bytes < AUDIO_CHUNK_SIZE ? operating_buf->remaining_bytes : AUDIO_CHUNK_SIZE;
      // Pull chip select low
      hw_digital_write(operating_buf->data_select, 0);
      // Transfer the data
      hw_spi_transfer_sync(SPI_PORT, &(operating_buf->buffer[operating_buf->buffer_position]), NULL, to_send, NULL);
      // Update our buffer position
      operating_buf->buffer_position += to_send;
      // Reduce the number of bytes remaining
      operating_buf->remaining_bytes -= to_send;
      // Pull chip select back up
      hw_digital_write(operating_buf->data_select, 1);
    }

    // If there are no bytes left
    if (operating_buf->remaining_bytes <= 0) {
      // Shift the buffer and emit the finished event
      shift_buffer();
    }
    else {
      // Wait for dreq to go high again
      audio_watch_dreq();
    }
  }
}

static void audio_watch_dreq() {
  // Start watching dreq for a low signal
  if (hw_digital_read(operating_buf->dreq)) {
    // Continue sending data
    audio_continue_spi();
  }
  // If not
  else {
    // Wait for dreq to go high
    hw_interrupt_watch(operating_buf->dreq, 1 << TM_INTERRUPT_MODE_HIGH, operating_buf->interrupt, audio_continue_spi);
  }
}

static void audio_emit_completion(uint16_t stream_id) {
  #ifdef DEBUG
  TM_DEBUG("Emitting completion"); 
  #endif
  lua_State* L = tm_lua_state;
  if (!L) return;

  lua_getglobal(L, "_colony_emit");
  lua_pushstring(L, "audio_playback_complete");
  lua_pushnumber(L, spi_async_status.transferError);
  lua_pushnumber(L, stream_id);
  tm_checked_call(L, 3);
}

static uint16_t generate_stream_id() {
  return ++master_id_gen;
}

// SPI.initialize MUST be initialized before calling this func
int8_t audio_play_buffer(uint8_t command_select, uint8_t data_select, uint8_t dreq, const uint8_t *buffer, uint32_t buf_len) {

  #ifdef DEBUG
  TM_DEBUG("Playing buffer, Current state:%d", current_state); 
  #endif


  if (operating_buf && current_state != STOPPED && current_state != RECORDING) {
    audio_stop_buffer(true);
  }

  return audio_queue_buffer(command_select, data_select, dreq, buffer, buf_len);
}

// SPI.initialize MUST be initialized before calling this func
int8_t audio_queue_buffer(uint8_t command_select, uint8_t data_select, uint8_t dreq, const uint8_t *buffer, uint32_t buf_len) {

  // If we are in the midst of recording, and play/queue is called, return an error
  if (current_state == RECORDING) {
    return -1;
  }

  #ifdef DEBUG
  TM_DEBUG("Queueing buffer, Args %d %d %d %d", command_select, data_select, dreq, buf_len); 
  #endif
  // Create a new buffer struct
  AudioBuffer *new_buf = malloc(sizeof(AudioBuffer));
  // Set the next field
  new_buf->next = NULL;
  // Malloc the space for the txbuf
  new_buf->buffer = malloc(buf_len);

  #ifdef DEBUG
  TM_DEBUG("Buffer Address %p", new_buf->buffer); 
  #endif

  // If the malloc failed, return an error
  if (new_buf->buffer == NULL) {
    free(new_buf);
    return -3;
  }

  // Copy over the bytes from the provided buffer
  memcpy(new_buf->buffer, buffer, buf_len);
  // Set the length field
  new_buf->remaining_bytes = buf_len;
  new_buf->buffer_position = 0;

  // Set the chip select field
  new_buf->data_select = data_select;
  // Write the data select as high initially
  // if we don't have an operation going on already 
  if (!operating_buf) {
    hw_digital_write(new_buf->data_select, 1);  
  }
  // Set the chip select pin as an output
  hw_digital_output(new_buf->data_select);

  // Set the command select field
  new_buf->command_select = command_select;
  // Write the command select as high initially
  // if we don't have an operation going on already 
  if (!operating_buf) {
    hw_digital_write(new_buf->command_select, 1);  
  }
  // Set the command select pin as an output
  hw_digital_output(new_buf->command_select);
  
  // Set the dreq field
  new_buf->dreq = dreq;
  // Set DREQ as an input
  hw_digital_input(new_buf->dreq);

  // Initialize SPI to the correct settings
  hw_spi_initialize(SPI_PORT, 4000000, HW_SPI_MASTER, HW_SPI_LOW, HW_SPI_FIRST, HW_SPI_FRAME_NORMAL);
  
  // If we have an existing operating buffer
  if (operating_buf) {
    #ifdef DEBUG
    TM_DEBUG("Using same interrupt %d", operating_buf->interrupt); 
    #endif
    // Use the same interrupt 
    new_buf->interrupt = operating_buf->interrupt;
  }
  // If this is the first audio buffer
  else {
    #ifdef DEBUG
    TM_DEBUG("Generating new interrupt..."); 
    #endif
    // Check if there are any more interrupts available
    if (!hw_interrupts_available()) {
      #ifdef DEBUG
      TM_DEBUG("Error: Unable to generate new interrupt!"); 
      #endif
      // If not, free the allocated memory
      free(new_buf->buffer);
      free(new_buf);
      // Return an error code
      return -2;
    }
    else {
      // If there are, grab the next available interrupt
      new_buf->interrupt = hw_interrupt_acquire();
      #ifdef DEBUG
      TM_DEBUG("New Interrupt acquired %d", new_buf->interrupt); 
      #endif

    }
  }

  // Generate an ID for this stream so we know when it's complete
  // We have to assign it to a local value before the struct
  // because the struct may be dealloc'ed before _queue_buffer returns
  uint8_t stream_id = generate_stream_id();
  new_buf->stream_id = stream_id;

  current_state = PLAYING;

  // It is possible for this to simply return if
  // The number of queued bytes is small enough 
  // that it doesn't have to wait for a gpio interrupt
  queue_buffer(new_buf);

  return stream_id;
}

static void sendEndFillBytes(uint32_t num_fill) {
  uint8_t chunk_size = 32;
  writeSciRegister16(VS1053_REG_WRAMADDR, VS1053_FILL_BYTE_ADDR);
  // Read the fill byte word and then chop off the high bytes
  uint8_t fill_byte = (uint8_t)readSciRegister16(VS1053_REG_WRAM);

  uint8_t *bytes = malloc(chunk_size * sizeof(uint8_t));

  for (uint8_t i = 0; i < chunk_size; i++) {
    bytes[i] = fill_byte;
  }

  uint32_t sent = 0;
  // While dreq is held high and we have more bytes to send
  while (sent < num_fill) {
    // Pull chip select low
    hw_digital_write(operating_buf->data_select, 0);
    // Transfer the data
    hw_spi_transfer_sync(SPI_PORT, bytes, NULL, chunk_size, NULL);
    // Pull chip select back up
    hw_digital_write(operating_buf->data_select, 1);

    sent+=chunk_size;
  }
}

// The force parameter indicates whether the final packet should
// be stopped right away or be allowed to complete streaming. 
int8_t audio_stop_buffer(bool immediately) {

  #ifdef DEBUG
  TM_DEBUG("Stopping buffer. Current state:%d", current_state); 
  #endif

  if (!operating_buf || current_state == RECORDING) {
    return -1;
  }

  // Remove interrupts
  audio_pause_buffer();

  if (immediately) {

    // Send 2052 fill bytes
    // (see section 9.11 of datasheet)
    sendEndFillBytes(2052);

    // Set the cancel bit
    uint16_t mode = readSciRegister16(VS1053_REG_MODE);
    writeSciRegister16(VS1053_REG_MODE, mode | (VS1053_MODE_SM_CANCEL));

    // Send 32 more fill bytes
    sendEndFillBytes(32);

    uint16_t fill_sent = 0;
    // Continue sending fill bytes until SM_ANCEL bit is cleared or we need to soft reset
    // Section 9.5.1 of the datasheet
    while (readSciRegister16(VS1053_REG_MODE) & VS1053_MODE_SM_CANCEL) {
      sendEndFillBytes(32);
      fill_sent += 32;
      if (fill_sent >= 2048) {
        softReset();
        break;
      }
    }

    // While for the stream to finish
    while (readSciRegister16(VS1053_REG_HDAT0) != 0 && readSciRegister16(VS1053_REG_HDAT1) != 0){};

    // Stop SPI
    hw_spi_async_cleanup();

    // Clean out the buffer
    audio_flush_buffer();
  }

  current_state = STOPPED;

  return 0;

}

int8_t audio_pause_buffer() {

  #ifdef DEBUG
  TM_DEBUG("Pausing buffer"); 
  #endif

  if (!operating_buf || current_state == STOPPED) {
    return -1;
  }

  // Stop watching for DREQ (so SPI will stop continuing)
  hw_interrupt_unwatch(operating_buf->interrupt, 1 << TM_INTERRUPT_MODE_HIGH);

  current_state = PAUSED;

  return 0;
}

int8_t audio_resume_buffer() {
  #ifdef DEBUG
  TM_DEBUG("Resume buffer"); 
  #endif

  // Start watching for dreq going low
  if (!operating_buf || current_state != PAUSED ) {
    return -1;
  }

  current_state = PLAYING;

  hw_interrupt_watch(operating_buf->dreq, 1 << TM_INTERRUPT_MODE_HIGH, operating_buf->interrupt, audio_continue_spi);

  return 0;
}

uint8_t audio_get_state() {
  return (uint8_t)current_state;
}

int8_t audio_start_recording(uint8_t command_select, uint8_t dreq, const char *plugin_dir, uint8_t *fill_buf, uint32_t fill_buf_len, uint32_t buf_ref) {

  if (current_state != STOPPED) {
    return -3;
  }

  #ifdef DEBUG
  TM_DEBUG("Input to start recording: %d %d %s", command_select, dreq, plugin_dir);
  #endif

  // Create a new buffer struct
  AudioBuffer *recording = malloc(sizeof(AudioBuffer));
  // Set the next field
  recording->next = NULL;
  recording->buffer = NULL;
  recording->remaining_bytes = 0;
  recording->buffer = fill_buf;
  recording->buffer_ref = buf_ref;
  recording->remaining_bytes = fill_buf_len;
  double_buff = malloc(fill_buf_len);

  if (double_buff == NULL) {
    free(recording);
    return -1;
  }

  // Set the chip select field
  recording->data_select = 0;

  // Set the command select field
  recording->command_select = command_select;
  // Set the command select pin as an output
  hw_digital_output(recording->command_select);
  // Write the command select as high initially
  hw_digital_write(recording->command_select, 1);  
  
  // Set the dreq field
  recording->dreq = dreq;
  // Set DREQ as an input
  hw_digital_input(recording->dreq);

  operating_buf = recording;

  // Set the maximum clock rate
  writeSciRegister16(VS1053_REG_CLOCKF, 0xC000);

  // Clear the bass channel
  writeSciRegister16(VS1053_REG_BASS, 0x00);
  
  // Soft reset
  softReset();

  // Set our application address to 0
  writeSciRegister16(VS1053_SCI_AIADDR, 0x00);

  // // disable all interrupts except SCI
  writeSciRegister16(VS1053_REG_WRAMADDR, VS1053_INT_ENABLE);
  writeSciRegister16(VS1053_REG_WRAM, 0x02);

  #ifdef DEBUG
  TM_DEBUG("Loading plugin...");
  #endif
  uint16_t addr = loadPlugin(plugin_dir);

  #ifdef DEBUG
  TM_DEBUG("Plugin start addr %d", addr);
  #endif

  if (addr == 0xFFFF || addr != 0x34) {
    #ifdef DEBUG
      TM_DEBUG("It was an invalid file...");
    #endif
    return -2;
  }

  // Set the recording input to Line In (always)
  writeSciRegister16(VS1053_REG_MODE, VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_LINE1);

  /* Set Maximum Signal Level */
  writeSciRegister16(VS1053_SCI_AICTRL0, 1024);

  /* Enable the AGC */
  writeSciRegister16(VS1053_SCI_AICTRL1, 1024);

  /* Set the maximum gain amplification to midrange */
  /* (1024 = 1×, 65535 = 64×) */
  writeSciRegister16(VS1053_SCI_AICTRL2, 0);

  /* Turn off run-time controls */
  writeSciRegister16(VS1053_SCI_AICTRL3, 0);

  // Start the recording!
  writeSciRegister16(VS1053_SCI_AIADDR, addr);

  while (!hw_digital_read(recording->dreq));

  // Start a timer to read data every so often
  startRIT();

  current_state = RECORDING;

  return 0;
}

int8_t audio_stop_recording(bool flush) {

  if (!operating_buf || current_state != RECORDING) {
    return -1;
  }

  #ifdef DEBUG
  TM_DEBUG("Actually stopping recording"); 
  #endif

  // Tell the vs1053 to stop recording
  writeSciRegister16(VS1053_SCI_AICTRL3, readSciRegister16(VS1053_SCI_AICTRL3) | 1);

  // If we are not flushing the rest of the buffer
  // perhaps because the script was cancelled
  if (!flush) {

    // Stop the timer
    stopRIT();

    // Reset the vs1053
    softReset();

    // Free our buffers and lua references
    freeRecordingResources();

    // Set the state to stopped
    current_state = STOPPED;
  }
  // If we will flush the rest of the data
  // Set the state to cancelling and wait
  // for the buffer to finish being flushed
  else {
    current_state = CANCELLING;
  }

  return 0;
}

static void audio_recording_stop_complete() {

  // Stop the timer
  stopRIT();

  // Get the number of words to iterate through. Only the last one matters
  uint32_t wordsLeft = readSciRegister16(VS1053_REG_HDAT1);
  // By default we will write both bytes of the last word
  uint8_t toWrite = 2;
  uint16_t w;
  // While there are words to read
  while (wordsLeft--) {
    // Read the word
    w = readSciRegister16(VS1053_REG_HDAT0);
    // Set it to the same two bytes
    double_buff[0] = (uint8_t)(w >> 8);
    double_buff[1] = (uint8_t)(w & 0xFF);
  }

  // When there are no more words left
  if (!wordsLeft) {
    // Read the register twice
    readSciRegister16(VS1053_SCI_AICTRL3);
    w = readSciRegister16(VS1053_SCI_AICTRL3);
    // If the 2 bit is 1
    if (w & 4) {
      // Don't read the second byte of the last word
      toWrite = 1;
    }
  }

  // Copy the last byte or two into the buffer
  memcpy(operating_buf->buffer, double_buff, toWrite);

  // Reset the chip
  softReset();

  // Free our buffers and lua references
  freeRecordingResources();

  // Set the state to stopped
  current_state = STOPPED;

  lua_State* L = tm_lua_state;
  if (!L) return;

  // Emit the final data
  lua_getglobal(L, "_colony_emit");
  lua_pushstring(L, "audio_recording_complete");
  lua_pushnumber(L, toWrite);
  tm_checked_call(L, 2);
}

static void recording_register_check(void) {

  if (operating_buf) {
    // Read as many bytes as possible (or length of buffer)
    uint32_t num_read = read_recorded_data(double_buff, operating_buf->remaining_bytes);
    // If we read anything
    if (num_read && num_read != 0xFFFF) {
      // Copy the data into our fill buffer
      memcpy(operating_buf->buffer, double_buff, num_read);

      lua_State* L = tm_lua_state;
      if (!L) return;

      lua_getglobal(L, "_colony_emit");
      lua_pushstring(L, "audio_recording_data");
      lua_pushnumber(L, num_read);
      tm_checked_call(L, 2);
    }
    // Check if we're attempting to cancel and the cancel is complete
    else if (current_state == CANCELLING && (readSciRegister16(VS1053_SCI_AICTRL3) & (1 << 1))) {
      // Clean up
      audio_recording_stop_complete();
    }
  }
}

// Reads up to len 8-bit words of recorded data
// Returns the number of bytes read
size_t read_recorded_data(uint8_t *data, size_t len) {

  uint16_t totalBytesRead = 0;
  uint8_t dataNeededInBuffer = 2;
  uint32_t wordsWaiting;

  /* See if there is some data available */
  if ((wordsWaiting = readSciRegister16(VS1053_REG_HDAT1)) > dataNeededInBuffer) {

    /* Always leave at least one word unread if Ogg Vorbis format */
    wordsWaiting = ((wordsWaiting-1) < (len/2) ? (wordsWaiting-1) : (len/2));

    // Gather the read data
    for (uint32_t i=0; i<wordsWaiting; i++) {
      uint16_t w = readSciRegister16(VS1053_REG_HDAT0);
      data[totalBytesRead++] = (uint8_t)(w >> 8);
      data[totalBytesRead++] = (uint8_t)(w & 0xFF);
    }
  }
  
  return totalBytesRead;
}

static void startRIT(void) {

  RIT_Init(LPC_RITIMER);

  RIT_TimerConfig(LPC_RITIMER, REC_READ_PERIOD);

  NVIC_EnableIRQ(RITIMER_IRQn);
}

static void stopRIT(void) {

  RIT_Cmd(LPC_RITIMER, DISABLE);

  NVIC_DisableIRQ(RITIMER_IRQn);
}

void RIT_IRQHandler(void)
{
  RIT_GetIntStatus(LPC_RITIMER);

  recording_register_check();
}

static void softReset() {
  // Save the GPIO state
  writeSciRegister16(VS1053_REG_WRAMADDR, VS1053_GPIO_IDATA);
  uint16_t gpioState = readSciRegister16(VS1053_REG_WRAM);
  // Soft reset
  writeSciRegister16(VS1053_REG_MODE, VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_RESET);
  // Wait for the reset to finish
  hw_wait_us(2);
  // Wait for dreq to come high again
  while (!hw_digital_read(operating_buf->dreq)) {
    continue;
  }

  // Place the GPIOs as outputs once again
  setGPIOAsOutput();

  // Set the gpio state back to how it was
  writeSciRegister16(VS1053_REG_WRAMADDR, gpioState);
  writeSciRegister16(VS1053_REG_WRAM, VS1053_GPIO_ODATA);

}

static void setGPIOAsOutput() {
  // Set the GPIOs back to outputs
  writeSciRegister16(VS1053_REG_WRAMADDR, VS1053_GPIO_DDR);
  writeSciRegister16(VS1053_REG_WRAM, (1 << INPUT_GPIO) + (1 << OUTPUT_GPIO));
}

static void freeRecordingResources() {
  // If we have an operating buf
  if (operating_buf) {
    // Free the lua reference
    if (operating_buf->buffer_ref != LUA_NOREF) {
      luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, operating_buf->buffer_ref);
    }

    // Clean up any SPI references
    hw_spi_async_cleanup();

    // Free the operating buffer
    free(operating_buf);
    operating_buf = NULL;

    // Free the fill buffer
    free(double_buff);
  }
}
// Code modified from Adafruit's lib
static uint16_t loadPlugin(const char *plugin_dir) {

  if (operating_buf) {

    // Generate a file handle for the plugin
    tm_fs_file_handle fd;

    // Open the file from the root directory
    int success = tm_fs_open(&fd, tm_fs_root, plugin_dir, TM_RDONLY);

    if (success != 0) {

      return 0xFFFF;
    }

    #ifdef DEBUG
    TM_DEBUG("The file was opened...");
    #endif

    // Grab a pointer to the file
    const uint8_t *plugin = tm_fs_contents(&fd);
    const uint32_t length = tm_fs_length(&fd);
    // Make sure the length is at least 3
    if (length < 3) {
      tm_fs_close(&fd);
      return 0xFFFF;
    }

    const uint32_t end = (uint32_t)plugin + length;

    if (*plugin++!= 'P' || *plugin++ != '&' || *plugin++ != 'H') {
      tm_fs_close(&fd);
      return 0xFFFF;
    }

    #ifdef DEBUG
    TM_DEBUG("It is a valid image.");
    #endif

    uint16_t type;

    while ((uint32_t)plugin < end) {
      type = *plugin++; 
      uint16_t offsets[] = {0x8000UL, 0x0, 0x4000UL};
      uint16_t addr, len;

      if (type >= 4) {
        tm_fs_close(&fd);
        return 0xFFFF;
      }

      len = *plugin++;    len <<= 8;
      len |= *plugin++ & ~1;
      addr = *plugin++;    addr <<= 8;
      addr |= *plugin++;

      if (type == 3) {
        // execute rec!
        tm_fs_close(&fd);
        return addr;
      }

      // set address
      writeSciRegister16(VS1053_REG_WRAMADDR, addr + offsets[type]);

      // write data
      do {
        uint16_t data;
        data = *plugin++;    data <<= 8;
        data |= *plugin++;
        writeSciRegister16(VS1053_REG_WRAM, data);
      } while ((len -=2));
    }

    tm_fs_close(&fd);
  }
  return 0xFFFF;
}

static uint16_t readSciRegister16(uint8_t address_byte) {
  uint16_t reg_val = 0;

  if (operating_buf) {

    //Wait for DREQ to go high indicating IC is available
    while (!hw_digital_read(operating_buf->dreq)) ; 

    // Assert the control line
    hw_digital_write(operating_buf->command_select, 0);

    //SCI consists of instruction byte, address byte, and 16-bit data word.

    const uint8_t tx[] = {VS1053_SCI_READ, address_byte, 0xFF, 0xFF};
    uint8_t rx[sizeof(tx)];

    // Send dummy data and read the response
    hw_spi_transfer_sync(SPI_PORT, tx, rx, sizeof(tx), NULL);

    // Wait for DREQ to go high indicating command is complete
    while (!hw_digital_read(operating_buf->dreq));

     // De-assert the control line
    hw_digital_write(operating_buf->command_select, 1);

    // Return the last two bytes as a 16 bit number
    reg_val = (rx[2] << 8) | rx[3];

  }

  return reg_val;
}

static void writeSciRegister16(uint8_t address_byte, uint16_t data) {

  if (operating_buf) {
    // Wait for DREQ to go high indicating IC is available
    while (!hw_digital_read(operating_buf->dreq)) ; 

    // Assert the control line
    hw_digital_write(operating_buf->command_select, 0);

    //SCI consists of instruction byte, address byte, and 16-bit data word.
    const uint8_t tx[] = {VS1053_SCI_WRITE, address_byte, data >> 8, data & 0xFF};

    // Send provided data to the register
    hw_spi_transfer_sync(SPI_PORT, tx, NULL, sizeof(tx), NULL);

    // Wait for DREQ to go high indicating command is complete
    while (!hw_digital_read(operating_buf->dreq));

     // De-assert the control line
    hw_digital_write(operating_buf->command_select, 1);
  }
}

void audio_reset() {
  // If we are playing audio
  if (current_state == PAUSED || current_state == PLAYING) {
    // Stop it
    audio_stop_buffer(true);
  }
  // If we are recording audio
  else if (current_state == RECORDING || current_state == CANCELLING) {
    // Stop it
    audio_stop_recording(false);
  }
}