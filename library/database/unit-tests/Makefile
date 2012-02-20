BASE = ../../..
QUIET = @

TESTS = \
        print_test \
        file_helper_test

file_helper_test__INCLUDES = \
  . \
  ../src \
  ../../../bins/include

file_helper_test__SOURCES = \
  ../src/file_helper.c
  
print_test__INCLUDES = . \
                         ../src \
                         ../../../bins/include

print_test__SOURCES  = \
                       ../src/add_song.c \
                       ../src/database_print.c \
                       ../src/database_purge.c \
                       ../src/generic.c \
                       ../src/indexer.c \
                       ../../binary-tree-avl/src/binary-tree-avl.c \
                       ../../circular-buffer/src/circular-buffer.c \
                       ../src/next_song.c \
                       ../src/queued_next_song.c \
                       ../src/w_malloc.c

print_test__CFLAGS = \
  -Wno-pointer-to-int-cast \
  -Wno-int-to-pointer-cast

include ../../make/Makefile.unit-test
