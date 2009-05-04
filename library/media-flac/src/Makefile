BOARD     = CROONER_2_0
TARGET    = media-flac
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = \
    media-flac.h

SOURCES = \
    media-flac.c \
    bitstream.c \
    decoder.c \
    shndec.c \
    avr32.S \
    tables.c

CFLAGS = \
    -DBUILD_STANDALONE \
    -DBUILD_BIGENDIAN \
    -DARCH_AVR32 \
    -DCPU_AVR32 \
    -DCONFIG_ALIGN

OPTIMIZATIONS = -O3

include ../../make/Makefile.common
