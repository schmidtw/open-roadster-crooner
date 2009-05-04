BOARD      = CROONER_2_0
TARGET     = sdram
BASE       = ../..
# Flash memories: [{cfi|internal}@address,size]...
FLASH      = internal@0x80000000,512Kb
# Clock source to use when programming: [{xtal|extclk|int}]
PROG_CLOCK = xtal

SOURCES = main.c

LIBRARIES = bsp

include ../make/Makefile.common
