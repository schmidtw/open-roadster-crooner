cc_windows = i686-pc-mingw32-gcc
cc_linux = gcc

cflags = -Wall \
         -DPACKAGE_STRING="\"Crooner Updater Firmware: 123\""

objs_windows = $(SOURCES:.c=.o-win32)
objs_linux = $(SOURCES:.c=.o-linux)
includes = -Idfu-programmer

SOURCES = \
          dfu-programmer/atmel.c \
          dfu-programmer/dfu.c \
          dfu-programmer/intel_hex.c \
          dfu-programmer/main.c \
          dfu-programmer/util.c

all: object.c updater.exe updater

updater : $(objs_linux)
	$(cc_linux) $(cflags) -o $@ $(objs_linux) -L/usr/lib -lusb

updater.exe : $(objs_windows)
	$(cc_windows) -static $(cflags) -o $@ $(objs_windows) -lusb

%.o-win32: %.c
	$(cc_windows) -c $< -o $@ -nostdinc $(cflags) $(includes) -I/usr/lib/gcc/i686-pc-mingw32/4.4.0/include -I/usr/i686-pc-mingw32/sys-root/mingw/include

%.o-linux: %.c
	$(cc_linux) -c $< -o $@ $(cflags) $(includes)

object.c : cd_changer.hex
	perl ./ihex2c.pl $< $@

clean:
	rm -f */*.o-win32 updater.exe */*.o-linux updater object.c