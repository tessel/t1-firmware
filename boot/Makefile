# Copyright (c) 2013 Technical Machine

OPTIM        = 0
RAM_MODE     = 0
BOARD_V      = 2

# RAM/ROM Config

ifeq ($(RAM_MODE),1)
	LD_SCRIPT    = ./ldscript_ram_gnu.ld
else
	LD_SCRIPT    = ./ldscript_rom_gnu.ld
endif

### Sources

CSRCS       += $(wildcard ../lpc18xx/Drivers/source/*.c)
CSRCS       += $(wildcard ../src/variants/lpc18xx/*.c)
CSRCS       += $(wildcard ../src/core/arduino/*.c)
CSRCS       += ../src/hw/spi_flash.c ../src/hw/system_lpc18xx.c ../src/hw/sbrk.c ../src/hw/startup.c ../src/hw/bootloader.c
CSRCS       += main.c usbdesc.c

ASRCS       += ../src/hw/startup_lpc1800.s

# Includes

INCDIR       = -I../lpc18xx/Drivers/include
INCDIR      += -I../lpc18xx/Core/CMSIS/Include
INCDIR      += -I../lpc18xx/Core/Include
INCDIR      += -I../lpc18xx/Core/Device/NXP/LPC18xx/Include
INCDIR      += -I../src/variants/lpc18xx
INCDIR      += -I../src/core/arduino/
INCDIR      += -I ../lpc18xx/usb_rom/
INCDIR      += -I ../src/hw/
INCDIR      += -I .

include ../common.mk