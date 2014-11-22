// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "tm.h"
#include "hw.h"
#include "string.h"
#include "colony.h"
#include "tessel.h"
#include <stdio.h>
#include <stdlib.h>
#include "lpc18xx_rit.h"

#define AUDIO_CHUNK_SIZE 32
#define SPI_PORT 0
#define REC_READ_PERIOD 300

#define VS1053_SCI_READ 0x03
#define VS1053_SCI_WRITE 0x02

#define VS1053_REG_MODE  0x00
#define VS1053_REG_STATUS 0x01
#define VS1053_REG_BASS 0x02
#define VS1053_REG_CLOCKF 0x03
#define VS1053_REG_DECODETIME 0x04
#define VS1053_REG_AUDATA 0x05
#define VS1053_REG_WRAM 0x06
#define VS1053_REG_WRAMADDR 0x07
#define VS1053_REG_HDAT0 0x08
#define VS1053_REG_HDAT1 0x09
#define VS1053_REG_VOLUME 0x0B

#define VS1053_GPIO_DDR 0xC017
#define VS1053_GPIO_IDATA 0xC018
#define VS1053_GPIO_ODATA 0xC019

#define VS1053_INT_ENABLE  0xC01A

#define VS1053_MODE_SM_DIFF 0x0001
#define VS1053_MODE_SM_LAYER12 0x0002
#define VS1053_MODE_SM_RESET 0x0004
#define VS1053_MODE_SM_CANCEL 0x0008
#define VS1053_MODE_SM_EARSPKLO 0x0010
#define VS1053_MODE_SM_TESTS 0x0020
#define VS1053_MODE_SM_STREAM 0x0040
#define VS1053_MODE_SM_SDINEW 0x0800
#define VS1053_MODE_SM_ADPCM 0x1000
#define VS1053_MODE_SM_LINE1 0x4000
#define VS1053_MODE_SM_CLKRANGE 0x8000

#define VS1053_FILL_BYTE_ADDR 0x1e06


#define VS1053_SCI_AIADDR 0x0A
#define VS1053_SCI_AICTRL0 0x0C
#define VS1053_SCI_AICTRL1 0x0D
#define VS1053_SCI_AICTRL2 0x0E
#define VS1053_SCI_AICTRL3 0x0F


#define TYPE_I 0
#define TYPE_X 1
#define TYPE_Y 2
#define TYPE_E 3

#define INPUT_GPIO 5
#define OUTPUT_GPIO 7

// Play a buffer. Will stop any currently playing buffer first.
int32_t audio_play_buffer(uint8_t command_select, uint8_t data_select, uint8_t dreq, const uint8_t *buf, int32_t ref, uint32_t buf_len);
// The same as play except it will append the buffer to a currently playing buffer if there is one (used for streams)
int32_t audio_queue_buffer(uint8_t command_select, uint8_t data_select, uint8_t dreq, const uint8_t *buf, int32_t ref, uint32_t buf_len);
// Stops any audio playing and frees memory
int8_t audio_stop_buffer(bool immediately);
// Stops music playback
int8_t audio_pause_buffer();
// Resumes music playback after pause
int8_t audio_resume_buffer();
// Get the current operating state of the mp3 player
uint8_t audio_get_state();
// Load a plugin and start recording sound
int8_t audio_start_recording(uint8_t command_select, uint8_t dreq, const char *plugin_dir, uint8_t *fill_buf, uint32_t fill_buf_len, uint32_t buf_ref);
// // Stop recording sound
int8_t audio_stop_recording(bool flush);

// Stop all playback and recording (used after ctrl+c)
void audio_reset();
