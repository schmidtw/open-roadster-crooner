BOARD      = CROONER_2_0
TARGET     = cd_changer
BASE       = ../..

SOURCES = \
          radio-interface.c \
          playback.c \
          main.c

LIBRARIES = \
            database \
            media-interface \
            media-flac \
            media-mp3 \
            file-stream \
            ibus-radio-protocol \
            ibus-physical \
            freertos \
            linked-list \
            display \
            memcard \
            dsp \
            bsp \
            util

OPTIMIZATIONS = -O0
SRAM_CODEC_SIZE=512
include $(BASE)/apps/make/Makefile.common
