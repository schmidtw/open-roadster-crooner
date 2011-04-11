BOARD   = CROONER_2_0
TARGET  = database
BASE     = ../../..
INCLUDE  = -I/usr/avr32/include \
           -I$(BASE)/bins/include

HEADERS = database.h

SOURCES = \
  add_song.c \
  generic.c \
  database_populate.c \
  database_print.c \
  database_purge.c \
  file_os_wrapper.c \
  indexer.c \
  init_database.c \
  mi_interface.c \
  next_song.c \
  w_malloc.c

include ../../make/Makefile.common
