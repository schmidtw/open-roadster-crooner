BOARD     = CROONER_2_0
TARGET    = freertos
BASE      = ../../..
INCLUDE   = -I/usr/avr32/include \
            -I$(BASE)/bins/include

HEADERS = \
    task.h \
    queue.h \
    semphr.h \
    portable.h \
    projdefs.h \
    FreeRTOSConfig.h \
    portmacro.h

SOURCES = \
    list.c \
    queue.c \
    tasks.c \
    port.c

LIBRARIES = \
            bsp

include ../../make/Makefile.common
