BOARD     = CROONER_2_0
TARGET    = memcard
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

CFLAGS    = -DCRC_FAST=1

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
    fatfs/ff.c \
    fatfs/ccsbcs.c \
    fatfs/avr32.c

include ../../make/Makefile.common
