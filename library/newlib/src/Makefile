BOARD   = CROONER_2_0
TARGET  = newlib
BASE    = ../../..
INCLUDE = -I/usr/avr32/include \
          -I$(BASE)/bins/include

HEADERS = reent-file-glue.h

STARTUP = \
    syscalls.c \
    crt0.S

include ../../make/Makefile.common
