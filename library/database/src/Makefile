BOARD   = CROONER_2_0
TARGET  = database
BASE     = ../../..
INCLUDE  = -I/usr/avr32/include \
           -I$(BASE)/bins/include

HEADERS = database.h

SOURCES = \
  add_song.c \
  album.c \
  artist.c \
  database_populate.c \
  database_print.c \
  database_purge.c \
  group.c \
  init_database.c \
  next_song.c \
  song.c \
  w_malloc.c

include ../../make/Makefile.common
