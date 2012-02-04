BOARD      = CROONER_2_0
TARGET     = cd_changer
BASE       = ../..

SOURCES = \
          user-interface.c \
          radio-interface.c \
          device-status.c \
          reboot-sys.c \
          ui-t.c \
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
            ibus-debug-protocol \
            ibus-physical \
            freertos \
            linked-list \
            circular-buffer \
            memcard \
            dsp \
            led \
            system-time \
            display \
            bsp \
            util

ENABLE_DISPLAY = 1

ifdef ENABLE_DISPLAY
LIBRARIES += display
CFLAGS += -DSUPPORT_TEXT=1
endif

OPTIMIZATIONS = -O0

include $(BASE)/apps/make/Makefile.common
