QUIET = @
BASE = ../../..

TESTS = \
        hma_test \
        hdu_test \
#        display_test \

display_test__INCLUDES = ../src .
hdu_test__INCLUDES     = ../src .
hma_test__INCLUDES     = ../src .

display_test__SOURCES  = ../src/display.c
hdu_test__SOURCES      = ../src/handle_display_update.c
hma_test__SOURCES      = ../src/handle_msg_action.c

display_test__MOCKS    = freertos

include ../../make/Makefile.unit-test
