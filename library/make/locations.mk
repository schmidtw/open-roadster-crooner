this_computer = $(shell uname -a)

TRENT_COMPUTER_NAMES = flexo
RAUL_COMPUTER_NAMES = 
WES_COMPUTER_NAMES = clouseau.357beelard cato.357beelard
STEVE_COMPUTER_NAMES =


ifneq "$(filter $(TRENT_COMPUTER_NAMES),$(this_computer))" ""
AVR32_INCLUDE = /opt/avr-headers
AVR32_COMPILER_INCLUDE = /opt/avr32-gnu-toolchain-linux_x86_64/avr32/include
else
ifneq "$(filter $(WES_COMPUTER_NAMES),$(this_computer))" ""
AVR32_INCLUDE = /opt/avr32-include
AVR32_COMPILER_INCLUDE = /usr/avr32/include
else
AVR32_INCLUDE = /opt/avr32-headers
AVR32_COMPILER_INCLUDE = /opt/avr32/include
endif
endif
