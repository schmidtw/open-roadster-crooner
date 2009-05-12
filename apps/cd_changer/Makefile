BOARD      = CROONER_2_0
TARGET     = cd_changer
BASE       = ../..

SOURCES = \
          ibus.c \
          blu.c \
          playback.c \
          main.c

LIBRARIES = \
            media-interface \
            media-flac \
            file-stream \
            ibus-physical \
            fatfs \
            freertos \
            linked-list \
            display \
            bsp \
            util

OPTIMIZATIONS = -O0
SRAM_CODEC_SIZE=512
include $(BASE)/apps/make/Makefile.common
