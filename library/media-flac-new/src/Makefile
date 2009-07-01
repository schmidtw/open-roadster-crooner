BOARD     = CROONER_2_0
TARGET    = flac
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = \

SOURCES = \
    format.c \
    fixed.c \
    float.c \
    bitreader.c \
    bitmath.c \
    md5.c \
    memory.c \
    lpc.c \
    cpu.c \
    crc.c \
    stream_decoder.c

CFLAGS = \
    -DVERSION=\"1.2.1\" \
    -DWORDS_BIGENDIAN=1

OPTIMIZATIONS = -O3

include ../../make/Makefile.common
