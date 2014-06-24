// Library for operating the Adafruit Neopixel strip

#include "tm.h"
#include "hw.h"
#include "variant.h"
#include "lpc18xx_sct.h"
#include <math.h> 

void basicTest ();

void writeColor(uint8_t pin, uint32_t color);

void LEDDRIVER_open (void);

/* Simple function to write to a transmit buffer.
 * NOTE: Application must keep track of how many data words have been sent!
 */
void LEDDRIVER_writeRGB (uint32_t rgb);

/* Activate or deactivate HALT after next frame. */
void LEDDRIVER_haltAfterFrame (int on);

/* Start a block transmission */
void LEDDRIVER_start (void);

/** Macro to define register bits and mask in CMSIS style */
#define LPCLIB_DefineRegBit(name,pos,width)    \
    enum { \
        name##_Pos = pos, \
        name##_Msk = (int)(((1ul << width) - 1) << pos), \
    }


/* Define SCT register bits */
LPCLIB_DefineRegBit(SCT_CONFIG_UNIFY,                   0,  1);
LPCLIB_DefineRegBit(SCT_CONFIG_CLKMODE,                 1,  2);
LPCLIB_DefineRegBit(SCT_CONFIG_CKSEL,                   3,  4);
LPCLIB_DefineRegBit(SCT_CONFIG_NORELOAD_L,              7,  1);
LPCLIB_DefineRegBit(SCT_CONFIG_NORELOAD_H,              8,  1);
LPCLIB_DefineRegBit(SCT_CONFIG_INSYNC,                  9,  8);
LPCLIB_DefineRegBit(SCT_CONFIG_AUTOLIMIT_L,             17, 1);
LPCLIB_DefineRegBit(SCT_CONFIG_AUTOLIMIT_H,             18, 1);

LPCLIB_DefineRegBit(SCT_CTRL_DOWN_L,                    0,  1);
LPCLIB_DefineRegBit(SCT_CTRL_STOP_L,                    1,  1);
LPCLIB_DefineRegBit(SCT_CTRL_HALT_L,                    2,  1);
LPCLIB_DefineRegBit(SCT_CTRL_CLRCTR_L,                  3,  1);
LPCLIB_DefineRegBit(SCT_CTRL_BIDIR_L,                   4,  1);
LPCLIB_DefineRegBit(SCT_CTRL_PRE_L,                     5,  8);
LPCLIB_DefineRegBit(SCT_CTRL_DOWN_H,                    16, 1);
LPCLIB_DefineRegBit(SCT_CTRL_STOP_H,                    17, 1);
LPCLIB_DefineRegBit(SCT_CTRL_HALT_H,                    18, 1);
LPCLIB_DefineRegBit(SCT_CTRL_CLRCTR_H,                  19, 1);
LPCLIB_DefineRegBit(SCT_CTRL_BIDIR_H,                   20, 1);
LPCLIB_DefineRegBit(SCT_CTRL_PRE_H,                     21, 8);

LPCLIB_DefineRegBit(SCT_CTRL_L_DOWN_L,                  0,  1);
LPCLIB_DefineRegBit(SCT_CTRL_L_STOP_L,                  1,  1);
LPCLIB_DefineRegBit(SCT_CTRL_L_HALT_L,                  2,  1);
LPCLIB_DefineRegBit(SCT_CTRL_L_CLRCTR_L,                3,  1);
LPCLIB_DefineRegBit(SCT_CTRL_L_BIDIR_L,                 4,  1);
LPCLIB_DefineRegBit(SCT_CTRL_L_PRE_L,                   5,  8);

LPCLIB_DefineRegBit(SCT_CTRL_H_DOWN_H,                  0,  1);
LPCLIB_DefineRegBit(SCT_CTRL_H_STOP_H,                  1,  1);
LPCLIB_DefineRegBit(SCT_CTRL_H_HALT_H,                  2,  1);
LPCLIB_DefineRegBit(SCT_CTRL_H_CLRCTR_H,                3,  1);
LPCLIB_DefineRegBit(SCT_CTRL_H_BIDIR_H,                 4,  1);
LPCLIB_DefineRegBit(SCT_CTRL_H_PRE_H,                   5,  8);

LPCLIB_DefineRegBit(SCT_EVx_CTRL_MATCHSEL,              0,  4);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_HEVENT,                4,  1);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_OUTSEL,                5,  1);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_IOSEL,                 6,  4);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_IOCOND,                10, 2);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_COMBMODE,              12, 2);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_STATELD,               14, 1);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_STATEV,                15, 5);
LPCLIB_DefineRegBit(SCT_EVx_CTRL_MATCHMEM,              20, 1);     /* Only in flash version! */
LPCLIB_DefineRegBit(SCT_EVx_CTRL_DIRECTION,             21, 2);     /* Only in flash version! */
