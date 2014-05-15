#include "tm.h"
#include "hw.h"
#include "string.h"
#include "colony.h"
#include "tessel.h"

// Will pass 32 byte chunks of a buffer to the audio module until completion
int audio_play_buffer(uint8_t command_select, uint8_t data_select, uint8_t dreq, const uint8_t *buf, uint32_t buf_len);
// Continue playing a buffer that has been paused
void audio_resume();
// Pause a buffer momentarily
void audio_pause();
// Stop playing a buffer free the memory
void audio_stop(); 
// Stops any audio playing and frees memory
void audio_clean();
