#include "tm.h"
#include "hw.h"
#include "string.h"
#include "colony.h"

int audio_play_buffer(uint8_t chip_select, uint8_t dreq, const uint8_t *buf, uint32_t buf_len);