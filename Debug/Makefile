# Copyright (c) 2013 Technical Machine

OPTIM        = fast
RAM_MODE     = 0
BOARD_V      = 2

PATH_RUNTIME = ../deps/runtime

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
CSRCS       += $(wildcard ../src/debug/*.c)
CSRCS       += $(wildcard ../src/core/bindings/*.c)
CSRCS       += $(wildcard ../src/tm/*.c)
CSRCS       += $(wildcard ../src/core/cc3000/*.c)
CSRCS       += $(wildcard ../src/hw/*.c)
CSRCS       += $(wildcard ../src/test/*.c)
CSRCS       += $(wildcard ../src/core/cc3000/utility/*.c)
CSRCS       += $(wildcard ../src/otp/*.c)
#CSRCS       += $(wildcard ../src/mmc/*.c)
CSRCS       += $(wildcard ../src/vcom/src/*.c)
CSRCS       += $(wildcard ../src/*.c)
CSRCS       += $(wildcard ../src/sdram/*.c)

CPPSRCS     += $(wildcard ../src/core/cc3000/*.cpp)
CPPSRCS     += $(wildcard ../src/tm/*.cpp)

ASRCS       += ../src/hw/startup_lpc1800.s

# Includes

INCDIR       = -I../lpc18xx/Drivers/include
INCDIR      += -I../lpc18xx/Core/CMSIS/Include
INCDIR      += -I../lpc18xx/Core/Include
INCDIR      += -I../lpc18xx/Core/Device/NXP/LPC18xx/Include
INCDIR      += -I$(PATH_RUNTIME)/src
INCDIR      += -I$(PATH_RUNTIME)/deps/fatfs/src
INCDIR      += -I$(PATH_RUNTIME)/deps/colony-lua/src
INCDIR      += -I$(PATH_RUNTIME)/deps/libtar/lib
INCDIR      += -I$(PATH_RUNTIME)/deps/libtar
INCDIR      += -I$(PATH_RUNTIME)/deps/libtar/compat
INCDIR      += -I$(PATH_RUNTIME)/deps/libtar/listhash
INCDIR      += -I$(PATH_RUNTIME)/deps/axtls/config
INCDIR      += -I$(PATH_RUNTIME)/deps/axtls/ssl
INCDIR      += -I$(PATH_RUNTIME)/deps/axtls/crypto
INCDIR      += -I../src
INCDIR      += -I../src/debug
INCDIR      += -I../src/otp
INCDIR      += -I../src/core/arduino
INCDIR      += -I../src/core/bindings
INCDIR      += -I../src/hw
INCDIR      += -I../src/tm
INCDIR      += -I../src/sdram
INCDIR      += -I../src/core/cc3000
INCDIR      += -I../src/core/cc3000/utility
INCDIR      += -I../src/vcom/inc
INCDIR      += -I../src/variants/lpc18xx

LIBFLAGS    += -L$(PATH_RUNTIME)/out/ARM
LIBS        += -ltm-arm

OTHER_TARGETS += build_runtime
OBJS += ../builtin/tessel.o

include ../common.mk

.PHONY: build_runtime
build_runtime:
	+make -C $(PATH_RUNTIME) arm

../builtin/tessel.c: ../builtin/tessel.js
	npm install
	../node_modules/.bin/colony -m ../builtin/tessel.js > ../builtin/tessel.lua
	colony -m ../builtin/tessel.js > ../builtin/tessel.lua
	(echo "const \\"; xxd -i ../builtin/tessel.lua) > ../builtin/tessel.c
