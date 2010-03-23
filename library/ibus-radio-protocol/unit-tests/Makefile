BASE = ../../..
QUIET = @

TESTS = \
        test_bcd \
        test_populate \
        test_message_converter \
        test_external

test_bcd__INCLUDES = . \
                     ../src

test_bcd__SOURCES  = \
                     ../src/bcd-track-converter.c

test_populate__INCLUDES = . \
                          ../src \
                          ../../../bins

test_populate__SOURCES  = . \
                          ../src/populate-message.c

test_message_converter__INCLUDES = . \
                                   ../src \
                                   ../../../bins

test_message_converter__SOURCES  = . \
                                   ../src/message-converter.c

test_external__INCLUDES = . \
                          ../src

test_external__SOURCES  = \
                          ../src/ibus-radio-protocol.c \
                          ../src/ibus-radio-to-string.c \
                          ../src/populate-message.c \
                          ../src/bcd-track-converter.c

include ../../make/Makefile.unit-test
