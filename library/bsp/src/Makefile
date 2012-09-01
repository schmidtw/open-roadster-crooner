BOARD   = CROONER_2_0
TARGET  = bsp
BASE    = ../../..
INCLUDE = -I/usr/avr32/include \
          -I$(BASE)/bins/include

CFLAGS = -DSTRICT_PARAMS=1

HEADERS = \
    bsp_errors.h \
    delay.h \
    cpu.h \
    gpio.h \
    pm.h \
    usart.h \
    intc.h \
    pdca.h \
    spi.h \
    wdt.h \
    flash.h \
    pwm.h \
    dac.h \
    reboot.h \
    boards/boards.h \
    boards/evk1100.h \
    boards/crooner-1.0.h \
    boards/crooner-2.0.h

SOURCES = \
    delay.c \
    cpu.c \
    gpio.c \
    pm.c \
    usart.c \
    exception.S \
    intc.c \
    pdca.c \
    spi.c \
    dac.c \
    reboot.c \
    flash.c \
    pwm.c \
    wdt.c

SKIP_STD_MOCK_IMPL = 1

include ../../make/Makefile.common
