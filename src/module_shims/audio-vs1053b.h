#include "tm.h"
#include "hw.h"
#include "string.h"
#include "colony.h"

// Will pass 32 byte chunks of a buffer to the audio module until completion
int audio_play_buffer(uint8_t chip_select, uint8_t dreq, const uint8_t *buf, uint32_t buf_len);
// Stops any audio playing and frees memory
void audio_clean();