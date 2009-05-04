# Hey Emacs, this is a -*- makefile -*-

# The purpose of this file is to define the build configuration variables used
# by the generic Makefile. See Makefile header for further information.

# Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/
# or other materials provided with the distribution.
#
# 3. The name of ATMEL may not be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
# SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ifndef SN
$(error "SN must be defined - format: SN=0x00000000")
endif

# CPU architecture: {ap|ucr1|ucr2}
ARCH = ucr2

# Part: {none|ap7xxx|uc3xxxxx}
PART = uc3a0512

# Flash memories: [{cfi|internal}@address,size]...
FLASH = internal@0x80000000,512Kb

# Clock source to use when programming: [{xtal|extclk|int}]
PROG_CLOCK = xtal

# Hardware version using the format: 0xabcd = ab.cd
CROONER_VERSION = 0x0200

# Target name: {*.a|*.elf}
TARGET = crooner-isp.elf

# Definitions: [-D name[=definition]...] [-U name...]
# Things that might be added to DEFS:
#   ISP_OSC           Oscillator that the ISP will use: {0|1}
#   CROONER_VERSION   Hardware version - see above
#   CROONER_SN        Part serial number - required from the commandline
DEFS = -D ISP_OSC=0 \
       -D CROONER_VERSION=$(CROONER_VERSION) \
       -D CROONER_SN=$(SN)

# ISP: general-purpose fuse bits - 7FFFF is important, the rest is not
ISP_GPFB = 0xFFF7FFFF

# Include path
INC_PATH = .

# C source files
CSRCS = \
        pm.c \
        flashc.c \
        usb_drv.c \
        usb_task.c \
        usb_device_task.c \
        usb_standard_request.c \
        isp.c \
        usb_dfu.c \
        intc.c \
        usb_descriptors.c \
        usb_specific_request.c

# Assembler source files
ASSRCS = boot.S

# Linker script file if any
LINKER_SCRIPT = ./link_at32uc3a-isp.lds

# Options to request or suppress warnings: [-fsyntax-only] [-pedantic[-errors]] [-w] [-Wwarning...]
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
WARNINGS = -Wall

# Options for debugging: [-g]...
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
DEBUG = -g

# Options that control optimization: [-O[0|1|2|3|s]]...
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
OPTIMIZATION = -Os -ffunction-sections -fdata-sections

# Extra flags to use when linking
LD_EXTRA_FLAGS = -Wl,--gc-sections -nostartfiles
