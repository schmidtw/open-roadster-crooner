/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


OUTPUT_FORMAT("elf32-avr32", "elf32-avr32", "elf32-avr32")

OUTPUT_ARCH(avr32:uc)

ENTRY(_start)

MEMORY
{
  LOW_BLANK      : ORIGIN = 0x80002000, LENGTH = 0x00000200
  FIRMWARE_LABEL : ORIGIN = 0x80002200, LENGTH = 0x00000040
  FLASH (rxai!w) : ORIGIN = 0x80002240, LENGTH = 0x00080000-0x2440
  HIGH_BLANK     : ORIGIN = 0x8007FE00, LENGTH = 0x00000200
  SDRAM (wxa!ri) : ORIGIN = 0xD0000000, LENGTH = 32M
  SYS_STACK (wxa!ri) : ORIGIN = 0x00000004, LENGTH = 0x00000040
  RESET_LOG (wxa!ri) : ORIGIN = 0x00000044, LENGTH = 0x00000100
  SRAM  (wxa!ri) : ORIGIN = 0x00000144, LENGTH = 0x0000FEBC
  USERPAGE       : ORIGIN = 0x80800000, LENGTH = 0x00000200
}

PHDRS
{
  FIRMWARE_LABEL PT_LOAD;
  LOW_BLANK      PT_LOAD;
  FLASH          PT_LOAD;
  HIGH_BLANK     PT_LOAD;
  SDRAM          PT_NULL;
  SYS_STACK      PT_NULL;
  SRAM           PT_NULL;
  SDRAM_AT_FLASH PT_LOAD;
  SRAM_AT_FLASH  PT_LOAD;
  USERPAGE       PT_LOAD;
}

SECTIONS
{
  /* If this heap size is selected, all the INTRAM space from the end of the
     data area to the beginning of the stack will be allocated for the heap. */
  __max_heap_size__ = -1;

  /* Use a default heap size if heap size was not defined. */
  __heap_size__ = DEFINED(__heap_size__) ? __heap_size__ : __max_heap_size__;

  /* Use a default stack size if stack size was not defined. */
  __stack_size__ = DEFINED(__stack_size__) ? __stack_size__ : 4K;

  /* Use a default sdram codec memory size if not defined. */
  __sdram_codec_size__ = DEFINED(__sdram_codec_size__) ? __sdram_codec_size__ : 0;

  /* Use a default sram codec memory size if not defined. */
  __sram_codec_size__ = DEFINED(__sram_codec_size__) ? __sram_codec_size__ : 0;

  /* Use sram for the nano buffer if defined */
  __nano_buffer_size__ = DEFINED(__nano_buffer_size__) ? __nano_buffer_size__ : 0;

  /* Read-only sections, merged into text segment: */
  PROVIDE (__executable_start = 0x80002200); . = 0x80002200;
  .interp           : { *(.interp) } >FLASH AT>FLASH :FLASH
  .reset            : { *(.reset) } >FLASH AT>FLASH :FLASH
  .hash             : { *(.hash) } >FLASH AT>FLASH :FLASH
  .dynsym           : { *(.dynsym) } >FLASH AT>FLASH :FLASH
  .dynstr           : { *(.dynstr) } >FLASH AT>FLASH :FLASH
  .gnu.version      : { *(.gnu.version) } >FLASH AT>FLASH :FLASH
  .gnu.version_d    : { *(.gnu.version_d) } >FLASH AT>FLASH :FLASH
  .gnu.version_r    : { *(.gnu.version_r) } >FLASH AT>FLASH :FLASH
  .rel.init         : { *(.rel.init) } >FLASH AT>FLASH :FLASH
  .rela.init        : { *(.rela.init) } >FLASH AT>FLASH :FLASH
  .rel.text         : { *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*) } >FLASH AT>FLASH :FLASH
  .rela.text        : { *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*) } >FLASH AT>FLASH :FLASH
  .rel.fini         : { *(.rel.fini) } >FLASH AT>FLASH :FLASH
  .rela.fini        : { *(.rela.fini) } >FLASH AT>FLASH :FLASH
  .rel.rodata       : { *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*) } >FLASH AT>FLASH :FLASH
  .rela.rodata      : { *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*) } >FLASH AT>FLASH :FLASH
  .rel.data.rel.ro  : { *(.rel.data.rel.ro*) } >FLASH AT>FLASH :FLASH
  .rela.data.rel.ro : { *(.rel.data.rel.ro*) } >FLASH AT>FLASH :FLASH
  .rel.data         : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) } >FLASH AT>FLASH :FLASH
  .rela.data        : { *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*) } >FLASH AT>FLASH :FLASH
  .rel.tdata        : { *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*) } >FLASH AT>FLASH :FLASH
  .rela.tdata       : { *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*) } >FLASH AT>FLASH :FLASH
  .rel.tbss         : { *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*) } >FLASH AT>FLASH :FLASH
  .rela.tbss        : { *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*) } >FLASH AT>FLASH :FLASH
  .rel.ctors        : { *(.rel.ctors) } >FLASH AT>FLASH :FLASH
  .rela.ctors       : { *(.rela.ctors) } >FLASH AT>FLASH :FLASH
  .rel.dtors        : { *(.rel.dtors) } >FLASH AT>FLASH :FLASH
  .rela.dtors       : { *(.rela.dtors) } >FLASH AT>FLASH :FLASH
  .rel.got          : { *(.rel.got) } >FLASH AT>FLASH :FLASH
  .rela.got         : { *(.rela.got) } >FLASH AT>FLASH :FLASH
  .rel.bss          : { *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*) } >FLASH AT>FLASH :FLASH
  .rela.bss         : { *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*) } >FLASH AT>FLASH :FLASH
  .rel.plt          : { *(.rel.plt) } >FLASH AT>FLASH :FLASH
  .rela.plt         : { *(.rela.plt) } >FLASH AT>FLASH :FLASH
  .init             :
  {
    KEEP (*(.init))
  } >FLASH AT>FLASH :FLASH =0xd703d703
  .plt              : { *(.plt) } >FLASH AT>FLASH :FLASH
  .text             :
  {
    *(.text .stub .text.* .gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
  } >FLASH AT>FLASH :FLASH =0xd703d703
  .fini             :
  {
    KEEP (*(.fini))
  } >FLASH AT>FLASH :FLASH =0xd703d703
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  .rodata           : { *(.rodata .rodata.* .gnu.linkonce.r.*) } >FLASH AT>FLASH :FLASH
  .rodata1          : { *(.rodata1) } >FLASH AT>FLASH :FLASH
  .eh_frame_hdr     : { *(.eh_frame_hdr) } >FLASH AT>FLASH :FLASH
  .eh_frame         : ONLY_IF_RO { KEEP (*(.eh_frame)) } >FLASH AT>FLASH :FLASH
  .gcc_except_table : ONLY_IF_RO { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) } >FLASH AT>FLASH :FLASH
  .lalign           : { . = ALIGN(8); PROVIDE(_data_lma = .); } >FLASH AT>FLASH :FLASH
  . = ORIGIN(SDRAM);
  .dalign           : { . = ALIGN(8); PROVIDE(_data = .); } >SDRAM
  /* Exception handling  */
  .eh_frame         : ONLY_IF_RW { KEEP (*(.eh_frame)) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .gcc_except_table : ONLY_IF_RW { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  /* Thread Local Storage sections  */
  .tdata            : { *(.tdata .tdata.* .gnu.linkonce.td.*) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .tbss             : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  PROVIDE (__preinit_array_start = ALIGN(32 / 8));
  .preinit_array    : { KEEP (*(.preinit_array)) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  PROVIDE (__preinit_array_end = .);
  PROVIDE (__init_array_start = .);
  .init_array       : { KEEP (*(.init_array)) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  PROVIDE (__init_array_end = .);
  PROVIDE (__fini_array_start = .);
  .fini_array       : { KEEP (*(.fini_array)) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  PROVIDE (__fini_array_end = .);
  .ctors            :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin*.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend*.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .dtors            :
  {
    KEEP (*crtbegin*.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend*.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .jcr              : { KEEP (*(.jcr)) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .data.rel.ro : { *(.data.rel.ro.local) *(.data.rel.ro*) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .dynamic          : { *(.dynamic) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .got              : { *(.got.plt) *(.got) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .ddalign          : { . = ALIGN(8); } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .data             :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    KEEP (*(.gnu.linkonce.d.*personality*))
    SORT(CONSTRUCTORS)
  } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .data1            : { *(.data1) } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  .balign           : { . = ALIGN(8); PROVIDE(_edata = .); } >SDRAM AT>FLASH :SDRAM_AT_FLASH
  PROVIDE (edata = .);
  __bss_start = .;
  .bss              :
  {
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    /* Align here to ensure that the .bss section occupies space up to
       _end.  Align after .bss to ensure correct alignment even if the
       .bss section disappears because there are no input sections.  */
    . = ALIGN(8);
  } >SDRAM AT>SDRAM :SDRAM
  . = ALIGN(8);
  _end = .;
  PROVIDE (end = .);
  __sdram_codec_start__ = ALIGN(8);
  .sdram_codec      :
  {
    *(.sdram_codec)
    . = __sdram_codec_size__;
    . = ALIGN(8);
  } >SDRAM AT>SDRAM : SDRAM
  __sdram_codec_end__ = .;
  __heap_start__ = ALIGN(8);
  .heap             :
  {
    *(.heap)
    . = (__heap_size__ == __max_heap_size__) ?
        ORIGIN(SDRAM) + LENGTH(SDRAM) - __sdram_codec_size__ - ABSOLUTE(.) :
        __heap_size__;
  } >SDRAM AT>SDRAM :SDRAM
  __heap_end__ = .;
  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  __reset_log__ = ORIGIN(RESET_LOG);
  __reset_log_size__ = LENGTH(RESET_LOG);
  __sp_stack_lower__ = ORIGIN(SYS_STACK);
  __sp_stack_upper__ = ORIGIN(SYS_STACK) + LENGTH(SYS_STACK);
  . = ORIGIN(SRAM);
  __nano_buffer_start__ = ALIGN(4);
  .nano_buffer      :
  {
    *(.nano_buffer)
    . = __nano_buffer_size__;
    . = ALIGN(8);
  } >SRAM AT>SRAM : SRAM
  __sram_codec_start__ = ALIGN(8);
  .sram_codec       :
  {
    *(.sram_codec)
    . = __sram_codec_size__;
    . = ALIGN(8);
  } >SRAM AT>SRAM : SRAM
  __sram_codec_end__ = .;
  __sram_heap_start__ = ALIGN(8);
  .sram_heap         :
  {
    *(.sram_heap)
    . = LENGTH(SRAM) - __stack_size__ - __nano_buffer_size__ - ABSOLUTE(.);
  } >SRAM AT>SRAM :SRAM
  __sram_heap_end__ = .;
  .stack         ORIGIN(SRAM) + LENGTH(SRAM) - __stack_size__ :
  {
    _stack = .;
    *(.stack)
    . = __stack_size__;
    _estack = .;
  } >SRAM AT>SRAM :SRAM
  .userpage       : { *(.userpage .userpage.*) } >USERPAGE AT>USERPAGE :USERPAGE
  /DISCARD/ : { *(.note.GNU-stack) }

  .firmware_label   : { KEEP(*(.firmware_label)) . = ALIGN(LENGTH(FIRMWARE_LABEL)); } >FIRMWARE_LABEL AT>FIRMWARE_LABEL :FIRMWARE_LABEL
  PROVIDE (__zeros_lo_start = ORIGIN(LOW_BLANK));
  .zeros_lo         : { KEEP(*(.zeros_lo)) . = 1; . = ALIGN(LENGTH(LOW_BLANK)); FILL(0x00000000);} >LOW_BLANK AT>LOW_BLANK :LOW_BLANK
  PROVIDE (__zeros_hi_start = ORIGIN(HIGH_BLANK));
  .zeros_hi         : { KEEP(*(.zeros_hi)) . = 1; . = ALIGN(LENGTH(HIGH_BLANK)); FILL(0x00000000);} >HIGH_BLANK AT>HIGH_BLANK :HIGH_BLANK


}
