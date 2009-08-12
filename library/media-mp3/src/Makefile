BOARD     = CROONER_2_0
TARGET    = media-mp3
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = \
    media-mp3.h

SOURCES = \
    media-mp3.c \
    bit.c \
    fixed.c \
    frame.c \
    huffman.c \
    layer12.c \
    layer3.c \
    stream.c \
    synth.c \
    timer.c \
    version.c \
    id3.c

CFLAGS = \
    -DFPM_AVR32=1 \
    -DHAVE_CONFIG_H=1

OPTIMIZATIONS = -O3

include ../../make/Makefile.common
