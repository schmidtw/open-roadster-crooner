QUIET = @
BASE = ../../..

TESTS = basic_tests

basic_tests__INCLUDES = ../src

basic_tests__SOURCES = ../src/led.c
basic_tests__CFLAGS = \
	-DLED_PWM_RED_PIN=50 \
	-DLED_PWM_RED_FUNCTION=100 \
	-DLED_PWM_RED_CHANNEL=12 \
	-DLED_PWM_GREEN_PIN=60 \
	-DLED_PWM_GREEN_FUNCTION=110 \
	-DLED_PWM_GREEN_CHANNEL=13 \
	-DLED_PWM_BLUE_PIN=70 \
	-DLED_PWM_BLUE_FUNCTION=120 \
	-DLED_PWM_BLUE_CHANNEL=14

basic_tests__MOCKS = \
    freertos \
    bsp \
	mock

include ../../make/Makefile.unit-test
