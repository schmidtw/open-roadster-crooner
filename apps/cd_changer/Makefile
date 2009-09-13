BOARD      = CROONER_2_0
TARGET     = cd_changer
BASE       = ../..

SOURCES = \
          radio-interface.c \
          device-status.c \
          main.c

LIBRARIES = \
            database \
            media-interface \
            media-flac \
            media-mp3 \
            playback \
            file-stream \
            ibus-radio-protocol \
            ibus-phone-protocol \
            ibus-physical \
            freertos \
            linked-list \
            display \
            memcard \
            dsp \
            led \
            bsp \
            util

OPTIMIZATIONS = -O0

include $(BASE)/apps/make/Makefile.common
