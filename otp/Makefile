# Copyright (c) 2013 Technical Machine

OPTIM        = fast
RAM_MODE     = 1

# RAM/ROM Config

ifeq ($(RAM_MODE),1)
	LD_SCRIPT    = ./ldscript_ram_gnu.ld
else
	LD_SCRIPT    = ./ldscript_rom_gnu.ld
endif

### Sources

CSRCS       += $(wildcard ../lpc18xx/Drivers/source/*.c)
CSRCS       += $(wildcard ../src/variants/lpc18xx/*.c)
CSRCS       += ../src/sys/system_lpc18xx.c ../src/sys/sbrk.c ../src/sys/startup.c
CSRCS       += main.c ./otp/otprom.c

ASRCS       += ../src/sys/startup_lpc1800.s

# Includes

INCDIR       = -I../lpc18xx/Drivers/include
INCDIR      += -I../lpc18xx/Core/CMSIS/Include
INCDIR      += -I../lpc18xx/Core/Include
INCDIR      += -I../lpc18xx/Core/Device/NXP/LPC18xx/Include
INCDIR      += -I../src/variants/lpc18xx
INCDIR      += -I../src/core/arduino/
INCDIR      += -I ../lpc18xx/usb_rom/
INCDIR      += -I ../src/
INCDIR      += -I ../src/hw/
INCDIR      += -I ../src/sys/
INCDIR      += -I ../src/cc3000/
INCDIR      += -I ./otp/
INCDIR      += -I .

include ../common.mk