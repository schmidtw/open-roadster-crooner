BASE = ../../..
QUIET = @

TESTS = \
        print_test \

print_test__INCLUDES = . \
                         ../src \
                         ../../../bins/include

print_test__SOURCES  = \
                       ../src/add_song.c \
                       ../src/album.c \
                       ../src/artist.c \
                       ../src/database_print.c \
                       ../src/database_purge.c \
                       ../src/group.c \
                       ../../linked-list/src/linked-list.c \
                       ../src/next_song.c \
                       ../src/song.c \
                       ../src/w_malloc.c

artist_test__INCLUDES = . \
                        ../src \
                        ../../../bins/include

artist_test__SOURCES  = \
                        ../src/artist.c \
                        ../src/group.c \
                        ../../linked-list/src/linked-list.c \

add_song_test__INCLUDES = . \
                          ../src \
                          ../../../bins/include

add_song_test__SOURCES  = \
                          ../src/add_song.c \
                          ../../linked-list/src/linked-list.c \

include ../../make/Makefile.unit-test
