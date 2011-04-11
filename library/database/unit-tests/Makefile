BASE = ../../..
QUIET = @

TESTS = \
        print_test \

print_test__INCLUDES = . \
                         ../src \
                         ../../../bins/include

print_test__SOURCES  = \
                       ../src/add_song.c \
                       ../src/database_print.c \
                       ../src/database_purge.c \
                       ../src/generic.c \
                       ../src/indexer.c \
                       ../../linked-list/src/linked-list.c \
                       ../src/next_song.c \
                       ../src/w_malloc.c

include ../../make/Makefile.unit-test
