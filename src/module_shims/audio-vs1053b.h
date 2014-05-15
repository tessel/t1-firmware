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
int8_t audio_stop_buffer();
// Stops music playback
int8_t audio_pause_buffer();
// Resumes music playback after pause
int8_t audio_resume_buffer();
// Get the current operating state of the mp3 player
uint8_t audio_get_state();
