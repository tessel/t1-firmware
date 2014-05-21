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
enum State { PLAYING, STOPPED, PAUSED, RECORDING };

// Callbacks for SPI and GPIO Interrupt
void _audio_spi_callback();
void _audio_continue_spi();

// Linked List queueing methods
void _queue_buffer(AudioBuffer *buffer);
void _shift_buffer();

// Internal Playback methods
void _audio_watch_dreq();
void _audio_emit_completion(uint16_t stream_id);
uint16_t _generate_stream_id();
void _audio_flush_buffer();

// Internal Recording methods
uint16_t _loadPlugin(const char *plugin_dir);
// Timer handler for reading data from the recording buffer
void RIT_IRQHandler(void);
void _recording_register_check();
void _startRIT(void);
void _stopRIT(void);
uint32_t _read_recorded_data(uint8_t *data, uint32_t len);

// Methods for controlling the device
void _writeSciRegister16(uint8_t address_byte, uint16_t data);
uint16_t _readSciRegister16(uint8_t address_byte);


tm_event shift_buffer_event = TM_EVENT_INIT(_shift_buffer);
tm_event read_recording_event = TM_EVENT_INIT(_recording_register_check);

// Buffer for loading data into before passing into runtime
uint8_t *double_buff = NULL;

AudioBuffer *operating_buf = NULL;
// Generates "unique" values for each pending stream
uint16_t master_id_gen = 0;
// The current state of the mp3 player
enum State current_state = STOPPED;

void _queue_buffer(AudioBuffer *buffer) {

  AudioBuffer **next = &operating_buf;
  // If we don't have an operating buffer already
  if (!(*next)) {
    // Assign this buffer to the head
    *next = buffer;

    #ifdef DEBUG
    TM_DEBUG("First buffer in queue. Beginning execution immediately."); 
    #endif

    tm_event_ref(&shift_buffer_event);

    // Start watching for a low DREQ
    _audio_watch_dreq();
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

void _shift_buffer() {
  #ifdef DEBUG
  TM_DEBUG("Shifting buffer."); 
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
      audio_stop_buffer();
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
      _audio_watch_dreq();
    }

    // Emit the event that we finished playing that buffer
    _audio_emit_completion(stream_id);
  }
}

void _audio_flush_buffer() {

  #ifdef DEBUG
  TM_DEBUG("Flushing buffer."); 
  #endif

  uint16_t mode = _readSciRegister16(VS1053_REG_MODE);

  _writeSciRegister16(VS1053_REG_MODE, mode | (VS1053_MODE_SM_CANCEL));

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

  tm_event_unref(&shift_buffer_event);
}
// Called when a SPI transaction has completed
void _audio_spi_callback() {

  // As long as we have an operating buffer
  if (operating_buf != NULL) {

    // If there was an error with the transfer
    if (spi_async_status.transferError) {
      // Set the number of bytes to continue writing to 0
      // This will force the completion event
      operating_buf->remaining_bytes = 0;
    }

    // Check if we have more bytes to send for this buffer
    if (operating_buf->remaining_bytes > 0) {
      // Send more bytes when DREQ is low
      _audio_watch_dreq();
    }
    // We've completed playing all the bytes for this buffer
    else {

      // We need to wait for the transaction to totally finish
      // The DMA interrupt is fired too early by NXP so we need to
      // poll the BSY register until we can continue
      while (SSP_GetStatus(LPC_SSP0, SSP_STAT_BUSY)){};

      // Pull chip select back up
      hw_digital_write(operating_buf->data_select, 1);

      // Remove this buffer from the linked list and free the memory
      tm_event_trigger(&shift_buffer_event);
    }
  }
}

// Called when DREQ goes low. Data is available to be sent over SPI
void _audio_continue_spi() {
  // If we have an operating buffer
  if (operating_buf != NULL) {
    // Figure out how many bytes we're sending next
    uint8_t to_send = operating_buf->remaining_bytes < AUDIO_CHUNK_SIZE ? operating_buf->remaining_bytes : AUDIO_CHUNK_SIZE;
    // Pull chip select low
    hw_digital_write(operating_buf->data_select, 0);
    // Transfer the data
    hw_spi_transfer(SPI_PORT, to_send, 0, &(operating_buf->buffer[operating_buf->buffer_position]), NULL, LUA_NOREF, LUA_NOREF, &_audio_spi_callback);
    // Update our buffer position
    operating_buf->buffer_position += to_send;
    // Reduce the number of bytes remaining
    operating_buf->remaining_bytes -= to_send;
  }
}

void _audio_watch_dreq() {
  // Start watching dreq for a low signal
  if (hw_digital_read(operating_buf->dreq)) {
    // Continue sending data
    _audio_continue_spi();
  }
  // If not
  else {
    // Wait for dreq to go low
    hw_interrupt_watch(operating_buf->dreq, TM_INTERRUPT_MODE_HIGH, operating_buf->interrupt, _audio_continue_spi);
  }
  
}

void _audio_emit_completion(uint16_t stream_id) {
  #ifdef DEBUG
  TM_DEBUG("Emitting completion"); 
  #endif
  lua_State* L = tm_lua_state;
  if (!L) return;

  lua_getglobal(L, "_colony_emit");
  lua_pushstring(L, "audio_complete");
  lua_pushnumber(L, spi_async_status.transferError);
  lua_pushnumber(L, stream_id);
  tm_checked_call(L, 3);
}

uint16_t _generate_stream_id() {
  return ++master_id_gen;
}

// SPI.initialize MUST be initialized before calling this func
int8_t audio_play_buffer(uint8_t command_select, uint8_t data_select, uint8_t dreq, const uint8_t *buffer, uint32_t buf_len) {

  #ifdef DEBUG
  TM_DEBUG("Playing buffer"); 
  #endif


  if (operating_buf && current_state != RECORDING) {
    audio_stop_buffer();
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
    return -3;
  }

  // Copy over the bytes from the provided buffer
  memcpy(new_buf->buffer, buffer, buf_len);
  // Set the length field
  new_buf->remaining_bytes = buf_len;
  new_buf->buffer_position = 0;

  // Set the chip select field
  new_buf->data_select = data_select;
  // Set the chip select pin as an output
  hw_digital_output(new_buf->data_select);
  // Write the data select as high initially
  // if we don't have an operation going on already 
  if (!operating_buf) {
    hw_digital_write(new_buf->data_select, 1);  
  }

  // Set the command select field
  new_buf->command_select = command_select;
  // Set the command select pin as an output
  hw_digital_output(new_buf->command_select);
  // Write the command select as high initially
  // if we don't have an operation going on already 
  if (!operating_buf) {
    hw_digital_write(new_buf->command_select, 1);  
  }
  
  // Set the dreq field
  new_buf->dreq = dreq;
  // Set DREQ as an input
  hw_digital_input(new_buf->dreq);

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
  new_buf->stream_id = _generate_stream_id();

  current_state = PLAYING;

  _queue_buffer(new_buf);

  return new_buf->stream_id;
}

int8_t audio_stop_buffer() {

  #ifdef DEBUG
  TM_DEBUG("Stopping buffer"); 
  #endif

  if (!operating_buf || current_state == RECORDING) {
    return -1;
  }

  uint16_t mode = _readSciRegister16(VS1053_REG_MODE);

  _writeSciRegister16(VS1053_REG_MODE, mode | (VS1053_MODE_SM_CANCEL));

    // Stop SPI
  hw_spi_async_cleanup();

  // Clear Interrupts
  hw_interrupt_unwatch(operating_buf->interrupt);

  // Clean out the buffer
  _audio_flush_buffer();

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
  hw_interrupt_unwatch(operating_buf->interrupt);

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

  hw_interrupt_watch(operating_buf->dreq, TM_INTERRUPT_MODE_HIGH, operating_buf->interrupt, _audio_continue_spi);

  return 0;
}

uint8_t audio_get_state() {
  return (uint8_t)current_state;
}

int8_t audio_start_recording(uint8_t command_select, uint8_t dreq, const char *plugin_dir, uint8_t *fill_buf, uint32_t fill_buf_len, uint32_t buf_ref, uint8_t mic_input) {

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
  _writeSciRegister16(VS1053_REG_CLOCKF, 0xC000);

  // Clear the bass channel
  _writeSciRegister16(VS1053_REG_BASS, 0x00);
  
  // Soft reset
  _writeSciRegister16(VS1053_REG_MODE, VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_RESET);
  // Wait for it to reset
  hw_wait_us(2);
    // Wait for dreq to come high again
  while (!hw_digital_read(operating_buf->dreq));

  // Set our application address to 0
  _writeSciRegister16(VS1053_SCI_AIADDR, 0x00);

  // // disable all interrupts except SCI
  _writeSciRegister16(VS1053_REG_WRAMADDR, VS1053_INT_ENABLE);
  _writeSciRegister16(VS1053_REG_WRAM, 0x02);

  #ifdef DEBUG
  TM_DEBUG("Loading plugin...");
  #endif
  uint16_t addr = _loadPlugin(plugin_dir);

  #ifdef DEBUG
  TM_DEBUG("Plugin start addr %d", addr);
  #endif

  if (addr == 0xFFFF || addr != 0x34) {
    TM_DEBUG("It was an invalid file...");
    return -2;
  }

  // Check if Mic or Line in
  if (mic_input) {
    _writeSciRegister16(VS1053_REG_MODE, VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW);
  }
  else {
    _writeSciRegister16(VS1053_REG_MODE, VS1053_MODE_SM_ADPCM | VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_LINE1);
  }

  /* Rec level: 1024 = 1. If 0, use AGC */
  _writeSciRegister16(VS1053_SCI_AICTRL0, 1024);

  /* Maximum AGC level: 1024 = 1. Only used if SCI_AICTRL1 is set to 0. */
  _writeSciRegister16(VS1053_SCI_AICTRL1, 1024);

  /* Miscellaneous bits that also must be set before recording. */
  _writeSciRegister16(VS1053_SCI_AICTRL2, 0);
  _writeSciRegister16(VS1053_SCI_AICTRL3, 0);

  // Start the recording!
  _writeSciRegister16(VS1053_SCI_AIADDR, 0x34);

  while (!hw_digital_read(recording->dreq));

  tm_event_ref(&read_recording_event);

  // Start a timer to read data every so often
  _startRIT();

  current_state = RECORDING;

  return 0;
}

int8_t audio_stop_recording() {

  if (!operating_buf || current_state != RECORDING) {
    return -1;
  }

  #ifdef DEBUG
  TM_DEBUG("Actually stopping recording"); 
  #endif

  // Stop the timer
  _stopRIT();

  // For stopping procedure, see http://www.vlsi.fi/fileadmin/software/VS10XX/vs1053-vorbis-encoder-170c.zip
  // Tell the vs1053 to stop gathering data by setting the first bit of VS1053_SCI_AICTRL3
  _writeSciRegister16(VS1053_SCI_AICTRL3, _readSciRegister16(VS1053_SCI_AICTRL3) | (1 << 0));

  // Keep reading until the 1st bit (not zeroth) of VS1053_SCI_AICTRL3 is flipped
  uint32_t num_read = _read_recorded_data(double_buff, operating_buf->remaining_bytes);

  // Read VS1053_SCI_AICTRL3 twice
  // If bit 2 is set in the second read, drop the last read byte
  _readSciRegister16(VS1053_SCI_AICTRL3);
  uint16_t drop = _readSciRegister16(VS1053_SCI_AICTRL3);

  if (num_read) {
    if (drop & (1 << 2)) {
      // The last byte we copy should only be the second to last byte
      double_buff[num_read-2] = double_buff[num_read-1];
      num_read--;
    }

    memcpy(operating_buf->buffer, double_buff, num_read);
  }

  // Soft reset
  _writeSciRegister16(VS1053_REG_MODE, VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_RESET);
  // Wait for the reset to finish
  hw_wait_us(2);
  // Wait for dreq to come high again
  while (!hw_digital_read(operating_buf->dreq)){};

  // If we have an operating buf (which we should)
  if (operating_buf) {
    // Free the lua reference
    if (operating_buf->buffer_ref != LUA_NOREF) {
      luaL_unref(tm_lua_state, LUA_REGISTRYINDEX, operating_buf->buffer_ref);
    }
    // Free the event reference
    if (read_recording_event.ref) {
      tm_event_unref(&read_recording_event);
    }

    current_state = STOPPED;

    // Free the operating buffer
    free(operating_buf);
    operating_buf = NULL;

    // Free the fill buffer
    free(double_buff);
    lua_State* L = tm_lua_state;

    if (!L) return -1;

    lua_getglobal(L, "_colony_emit");
    lua_pushstring(L, "audio_complete");
    lua_pushnumber(L, num_read);
    tm_checked_call(L, 2);
  }

  return 0;
}

void _recording_register_check(void) {

  if (operating_buf) {
    // Read as many bytes as possible (or length of buffer)
    uint32_t num_read = _read_recorded_data(double_buff, operating_buf->remaining_bytes);
    // If we read anything
    if (num_read && num_read != 0xFFFF) {
      // Copy the data into our fill buffer
      memcpy(operating_buf->buffer, double_buff, num_read);

      lua_State* L = tm_lua_state;
      if (!L) return;

      lua_getglobal(L, "_colony_emit");
      lua_pushstring(L, "audio_data");
      lua_pushnumber(L, num_read);
      tm_checked_call(L, 2);
    }
  }
}

// Reads up to len 8-bit words of recorded data
// Returns a 1 if there was more data available
uint32_t _read_recorded_data(uint8_t *data, uint32_t len) {

  uint32_t num_to_read = _readSciRegister16(VS1053_REG_HDAT1);

  // If there are more bytes to read than the size of the
  // allowed, we only read the allowed
  if (num_to_read > (len * 2)) {
    num_to_read = len/2;
  } 

  uint16_t raw;
  uint16_t i = 0;

  while (i < num_to_read) {
    raw = _readSciRegister16(VS1053_REG_HDAT0);

    data[i] = raw >> 8;
    data[i + 1] = raw & 0xFF;

    if (num_to_read-i > 1) {
      i+=2;
    }
    else {
      i++;
    }
  }

  return num_to_read;
}

void _startRIT(void) {

  RIT_Init(LPC_RITIMER);

  RIT_TimerConfig(LPC_RITIMER, REC_READ_PERIOD);

  NVIC_EnableIRQ(RITIMER_IRQn);
}

void _stopRIT(void) {

  RIT_Cmd(LPC_RITIMER, DISABLE);

  NVIC_DisableIRQ(RITIMER_IRQn);
}

void RIT_IRQHandler(void)
{
  RIT_GetIntStatus(LPC_RITIMER);
  tm_event_trigger(&read_recording_event);
}

// Code modified from Lada Ada's lib
uint16_t _loadPlugin(const char *plugin_dir) {

  if (operating_buf) {

    // Generate a file handle for the plugin
    tm_fs_file_handle fd;

    // Open the file from the root directory
    int success = tm_fs_open(&fd, tm_fs_root, plugin_dir, TM_RDONLY);

    if (success == -EEXIST) {
      return 0xFFFF;
    }

    #ifdef DEBUG
    TM_DEBUG("The file was opened...");
    #endif

    // Grab a pointer to the file
    const uint8_t *plugin = tm_fs_contents(&fd);
    const uint32_t end = (uint32_t)plugin + tm_fs_length(&fd);

    if (*plugin++!= 'P' || *plugin++ != '&' || *plugin++ != 'H') {
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
      _writeSciRegister16(VS1053_REG_WRAMADDR, addr + offsets[type]);

      // write data
      do {
        uint16_t data;
        data = *plugin++;    data <<= 8;
        data |= *plugin++;
        _writeSciRegister16(VS1053_REG_WRAM, data);
      } while ((len -=2));
    }

    tm_fs_close(&fd);
  }
  return 0xFFFF;
}

uint16_t _readSciRegister16(uint8_t address_byte) {
  uint16_t reg_val = 0;

  if (operating_buf) {

    //Wait for DREQ to go high indicating IC is available
    while (!hw_digital_read(operating_buf->dreq)) ; 

    // Assert the control line
    hw_digital_write(operating_buf->command_select, 0);

    //SCI consists of instruction byte, address byte, and 16-bit data word.

    const uint8_t tx[] = {VS1053_SCI_READ, address_byte, 0xFF, 0xFF};
    uint8_t rx[sizeof(tx)];

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

void _writeSciRegister16(uint8_t address_byte, uint16_t data) {

  if (operating_buf) {
    // Wait for DREQ to go high indicating IC is available
    while (!hw_digital_read(operating_buf->dreq)) ; 

    // Assert the control line
    hw_digital_write(operating_buf->command_select, 0);

    //SCI consists of instruction byte, address byte, and 16-bit data word.
    const uint8_t tx[] = {VS1053_SCI_WRITE, address_byte, data >> 8, data & 0xFF};

    hw_spi_transfer_sync(SPI_PORT, tx, NULL, sizeof(tx), NULL);

    // Wait for DREQ to go high indicating command is complete
    while (!hw_digital_read(operating_buf->dreq));

     // De-assert the control line
    hw_digital_write(operating_buf->command_select, 1);
  }
}

void audio_reset() {
  audio_stop_buffer();
  audio_stop_recording();
}