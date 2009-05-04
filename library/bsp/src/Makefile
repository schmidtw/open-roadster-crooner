BOARD   = CROONER_2_0
TARGET  = bsp
BASE    = ../../..
INCLUDE = -I/usr/avr32/include \
          -I$(BASE)/bins/include

CFLAGS = -DSTRICT_PARAMS=1

HEADERS = \
    abdac.h \
    bsp_errors.h \
    crc.h \
    delay.h \
    gpio.h \
    pm.h \
    usart.h \
    intc.h \
    pdca.h \
    spi.h \
    memcard.h \
    led.h \
    boards/boards.h \
    boards/evk1100.h \
    boards/crooner-1.0.h \
    boards/crooner-2.0.h

SOURCES = \
    crc.c \
    abdac.c \
    delay.c \
    gpio.c \
    pm.c \
    usart.c \
    exception.S \
    intc.c \
    pdca.c \
    spi.c \
    memcard.c \
    led.c

include ../../make/Makefile.common
