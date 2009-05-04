CHIP       = uc3a0512es
BOARD      = EVK1100
TARGET     = test
BASE       = ../..
QUIET      = @
# Flash memories: [{cfi|internal}@address,size]...
FLASH      = internal@0x80000000,512Kb
# Clock source to use when programming: [{xtal|extclk|int}]
PROG_CLOCK = xtal

cc       = avr32-gcc
nm       = avr32-nm
program  = avr32program
objs     = $(SOURCES:.c=.o)
sleepusb = 1

inc_dir = $(BASE)/bins/include
lib_dir = $(BASE)/bins/lib

lboard = $(shell echo $(BOARD) | tr "[:upper:]" "[:lower:]")
target_name = $(TARGET).elf
libraries = $(addsuffix -$(lboard),$(LIBRARIES))
last_goal = $(lastword $(MAKECMDGOALS))

cflags = \
    -march=ucr1 \
    -mpart=$(CHIP) \
    -Wall \
    -O0 \
    -ffunction-sections \
    -fdata-sections \
    -D__IEEE_BIG_ENDIAN \
    -fshort-enums \
    -DBOARD=$(BOARD)

link_script = -T ./link_uc3a0512.lds

SOURCES = \
          main.c

INCLUDES = \
           -I/usr/avr32/include \
           -I$(inc_dir)

LIBRARIES = \
            freertos \
            bsp

all :: $(target_name)

$(target_name) : $(objs) $(addsuffix .a,$(addprefix $(lib_dir)/lib,$(libraries)))
	$(QUIET)$(cc) $(cflags) $(link_script) -Wl,--gc-sections -L$(lib_dir) -o $@ $(objs) $(addprefix -l,$(libraries))
	$(QUIET)$(nm) -n $(target_name) > $(TARGET).sym

.c.o :
	$(QUIET)$(cc) $(cflags) -g -c $(INCLUDES) -o $@ $<

.PHONY: clean
clean ::
	$(QUIET)rm -f $(objs) $(target_name)

simple_jtag_commands = halt cpuinfo chiperase reset secureflash run
.PHONY: $(simple_jtag_commands)
$(simple_jtag_commands) :
	@echo "Performing: $@"
	$(QUIET)$(program) $@
ifneq ($(filter $(last_goal),$(simple_jtag_commands)),$(last_goal))
	$(QUIET)sleep $(sleepusb)
endif

.PHONY: program
program : halt $(target_name)
	@echo "Performing: $@"
	$(QUIET)$(program) program -f$(FLASH) -c$(PROG_CLOCK) -e -v -R $(if $(findstring run,$(MAKECMDGOALS)),-r) $(target_name)
ifneq ($(filter $(last_goal),program),$(last_goal))
	$(QUIET)sleep $(sleepusb)
endif
