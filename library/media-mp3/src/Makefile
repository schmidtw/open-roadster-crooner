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
    dct32_avr32.S \
    decoder.c \
    fixed.c \
    frame.c \
    huffman.c \
    imdct_avr32.S \
    layer12.c \
    layer3.c \
    stream.c \
    synth.c \
    synth_avr32.S \
    timer.c \
    version.c

CFLAGS = \
    -DOPT_SSO \
	-DOPT_STACK \
	-DHAVE_LIMITS_H \
	-DHAVE_ASSERT_H \
	-DWORDS_BIGENDIAN \
    -DFPM_AVR32

OPTIMIZATIONS = -O3

include ../../make/Makefile.common
