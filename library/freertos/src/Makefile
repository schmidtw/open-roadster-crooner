BOARD     = CROONER_2_0
TARGET    = freertos
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = os.h

SOURCES = \
    os.c \
    list.c \
    queue.c \
    tasks.c \
    port.c

EXTRA_HEADER = os-mock-extra.h

LIBRARIES = \
            bsp

include ../../make/Makefile.common
