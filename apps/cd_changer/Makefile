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
            binary-tree-avl \
            circular-buffer \
            memcard \
            dsp \
            led \
            system-time \
            system-log \
            display \
            bsp \
            fillable-buffer \
            util

ENABLE_DISPLAY = 0

ifeq "$(ENABLE_DISPLAY)" "1"
LIBRARIES += display
CFLAGS += -DSUPPORT_TEXT=1
endif

OPTIMIZATIONS = -O0

include $(BASE)/apps/make/Makefile.common
