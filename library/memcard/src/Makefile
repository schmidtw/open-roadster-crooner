BOARD     = CROONER_2_0
TARGET    = memcard
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = \
    memcard.h \
    dirent.h

SOURCES = \
    csd.c \
    io.c \
    crc.c \
    commands.c \
    command.c \
    memcard.c \
    memory-block.c \
    block.c \
    glue.c \
    dirent.c \
    debug-helpers.c \
    fatfs/ff.c \
    fatfs/avr32.c

include ../../make/Makefile.common
