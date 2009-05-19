BOARD     = CROONER_2_0
TARGET    = memcard
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = \
    memcard.h

SOURCES = \
    csd.c \
    io.c \
    crc.c \
    fast-fill.c \
    commands.c \
    command.c \
    memcard.c

include ../../make/Makefile.common
