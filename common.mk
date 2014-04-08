PREFIX       = arm-none-eabi
CPU          = cortex-m3

### Tools

CC      = $(PREFIX)-gcc
CPP     = $(PREFIX)-g++
LD      = $(PREFIX)-ld
AR      = $(PREFIX)-ar
AS      = $(PREFIX)-as
CP      = $(PREFIX)-objcopy
OD      = $(PREFIX)-objdump
SIZE    = $(PREFIX)-size
RM      = rm
Q       = @../Debug/quiet.js


### Compilation flags

CFLAGS      += -mcpu=$(CPU) 
CFLAGS      += -mthumb
CFLAGS      += -gdwarf-2 
CFLAGS      += -mtune=$(CPU)
CFLAGS      += -mlong-calls
CFLAGS      += -mfix-cortex-m3-ldrd
CFLAGS      += -msoft-float
CFLAGS      += -Wall 
CFLAGS      += -O$(OPTIM) 
CFLAGS      += -mapcs-frame 
CFLAGS      += -mno-sched-prolog 
CFLAGS      += -ffunction-sections 
CFLAGS      += -fdata-sections 
CFLAGS      += -fno-exceptions -fno-unwind-tables
CFLAGS      += $(CDEFFLAGS)
CFLAGS      += $(INCDIR)
CFLAGS      += -funsigned-char
CFLAGS      += -fno-strict-aliasing

CDEFFLAGS   += -DTESSEL_BOARD_V=$(BOARD_V)
CDEFFLAGS   += -D__thumb2__=1 
CDEFFLAGS   += -DGPIO_PIN_INT
CDEFFLAGS   += -D_UART2
CDEFFLAGS   += -DTM_FS_fat -D_POSIX_SOURCE
CDEFFLAGS   += -DREGEX_WCHAR=1
CDEFFLAGS   += -D__TESSEL_FIRMWARE_VERSION__='"$(shell git log --pretty=format:'%h' -n 1)"' 
CDEFFLAGS   += -D__TESSEL_RUNTIME_VERSION__='"$(shell git --git-dir ../deps/runtime/.git log --pretty=format:'%h' -n 1)"'
CDEFFLAGS   += -include ../deps/runtime/deps/axtls/config/embed/config.h

AFLAGS       = -mcpu=$(CPU) 
AFLAGS      += -mthumb
AFLAGS      += -ggdb 
AFLAGS      += $(INCDIR) 
AFLAGS      += --defsym RAM_MODE=$(RAM_MODE)

### Libraries

LIBS        += -lm
LIBS        += -lc
LIBS        += -lnosys

### Flags

ASFLAGS  =
LDFLAGS  = --gc-sections -marmelf
CPFLAGS  =
ODFLAGS  = -x --syms
PRFLAGS ?=

### Targets

OBJS += $(ASRCS:.s=.o) $(CSRCS:.c=.o) $(CPPSRCS:.cpp=.o)

.PHONY: all size clean nuke

all: main.bin

size: main.elf
	@$(SIZE) $<

%.hex: %.elf
	@$Q "HEX  $@" $(CP) $(CPFLAGS) -O ihex $< $*.hex

%.bin: %.elf
	@$Q "BIN  $@" $(CP) $(CPFLAGS) -O binary $< $*.bin

LINKEROPT = -Wl,

main.elf: $(LD_SCRIPT) $(OBJS) $(OTHER_TARGETS)
	@$Q "LINK $@" $(CC) $(CFLAGS) $(addprefix $(LINKEROPT),$(LDFLAGS)) -T $(LD_SCRIPT) $(OBJS) $(LIBFLAGS) -Wl,--start-group $(LIBS) -Wl,--end-group -o $@
	@$Q DUMP $(OD) $(ODFLAGS) $@ > $(@:.elf=.dump)
	@$(SIZE) $@

%.o: %.s
	@$Q "AS   $<" $(AS) $(AFLAGS) $< -o $*.o

%.o: %.c
	@$Q "DEP  $<" $(CC) -MM $< -MF $*.d -MP $(INCDIR) $(LIBFLAGS) $(LIBS) $(CDEFFLAGS)
	@$Q "CC   $<"  $(CC) -c $(CFLAGS) -std=c99 $< -o $@

%.o: %.cpp
	@$Q "DEP  $<" $(CPP) -MM $< -MF $*.d -MP $(INCDIR) $(LIBFLAGS) $(LIBS) $(CDEFFLAGS) 
	@$Q "CPP  $<" $(CPP) -c $(CFLAGS) $< -o $@  

clean:
	@-rm -f *.elf 
	@-\
for D in "." "../src/**" "../src/**/**" "../src/**/**/**" "../src/**/**/**/**" "../src/**/**/**/**/**" "../src/**/**/**/**/**/**" "../src/**/**/**/**/**/**/**" "../src/**/**/**/**/**/**/**/**" "../lpc18xx/Core/Device/NXP/LPC18xx/Source/Templates/GCC"; do \
	rm -f $$D/*.o $$D/*.d $$D/*.lst $$D/*.dump $$D/*.map; \
done

nuke: clean
	@-\
for D in "." "../**" "../**/**" "../**/**/**" "../**/**/**/**" "../**/**/**/**/**" "../**/**/**/**/**/**" "../**/**/**/**/**/**/**" "../**/**/**/**/**/**/**/**"; do \
	rm -f $$D/*.o $$D/*.d $$D/*.lst $$D/*.dump $$D/*.map; \
done
	-rm -f *.hex *.bin

-include $(CSRCS:.c=.d) $(CPPSRCS:.cpp=.d)
