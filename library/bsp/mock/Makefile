TARGET = bsp
BASE = ../../..

HEADERS = cpu-mock.h \
          delay-mock.h \
          flash-mock.h \
          gpio-mock.h \
          intc-mock.h \
          pdca-mock.h \
          pm-mock.h \
          pwm-mock.h \
          reboot-mock.h \
          spi-mock.h \
          usart-mock.h \
          wdt-mock.h

SOURCES = cpu-mock.c \
          delay-mock.c \
          flash-mock.c \
          gpio-mock.c \
          intc-mock.c \
          pdca-mock.c \
          pm-mock.c \
          pwm-mock.c \
          reboot-mock.c \
          spi-mock.c \
          usart-mock.c \
          wdt-mock.c

CFLAGS = -D__AVR32_UC3A1512__

SYSTEM_INCLUDES = \
  $(AVR32_INCLUDE) \
  $(AVR32_COMPILER_INCLUDE)

clean ::
	rm -f *.c *.h

include ../../make/Makefile.mock

