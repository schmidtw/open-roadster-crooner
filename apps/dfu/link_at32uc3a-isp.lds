/******************************************************************************
 * AVR32 AT32UC3A ISP GNU LD script file.
 *
 * - Compiler:           GNU GCC for AVR32
 * - Supported devices:  AVR32 AT32UC3A
 *
 * - author              Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/* Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */

OUTPUT_FORMAT("elf32-avr32", "elf32-avr32", "elf32-avr32")

OUTPUT_ARCH(avr32:uc)

ENTRY(_start)

MEMORY
{
  FLASH (rxai!w) : ORIGIN = 0x80000000, LENGTH = 0x00080000
  INTRAM (wxa!ri) : ORIGIN = 0x00000004, LENGTH = 0x0000FFFC
  USERPAGE : ORIGIN = 0x80800000, LENGTH = 0x00000200
}

PHDRS
{
  FLASH PT_LOAD;
  INTRAM_ALIGN PT_NULL;
  INTRAM_AT_FLASH PT_LOAD;
  INTRAM PT_NULL;
  CONST_INTRAM_AT_FLASH PT_LOAD;
  USERPAGE PT_LOAD;
}

SECTIONS
{
  /* Use a default heap size if heap size was not defined. */
  __heap_size__ = DEFINED(__heap_size__) ? __heap_size__ : 0;

  /* Use a default stack size if stack size was not defined. */
  __stack_size__ = DEFINED(__stack_size__) ? __stack_size__ : 1K;

  /* Read-only sections, merged into text segment: */
  PROVIDE (__executable_start = 0x80000000); . = 0x80000000;
  .interp         : { *(.interp) } >FLASH AT>FLASH :FLASH
  .reset : {  *(.reset) } >FLASH AT>FLASH :FLASH
  .lalign	: { . = ALIGN(8); PROVIDE(_data_lma = .); } >FLASH AT>FLASH :FLASH
  . = ORIGIN(INTRAM);
  .dalign	: { . = ALIGN(8); PROVIDE(_data = .); } >INTRAM AT>INTRAM :INTRAM_ALIGN
  /* Exception handling  */
  .eh_frame       : ONLY_IF_RW { KEEP (*(.eh_frame)) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .gcc_except_table   : ONLY_IF_RW { KEEP (*(.gcc_except_table)) *(.gcc_except_table.*) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  /* Thread Local Storage sections  */
  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  PROVIDE (__preinit_array_start = ALIGN(32 / 8));
  .preinit_array     : { KEEP (*(.preinit_array)) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  PROVIDE (__preinit_array_end = .);
  PROVIDE (__init_array_start = .);
  .init_array     : { KEEP (*(.init_array)) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  PROVIDE (__init_array_end = .);
  PROVIDE (__fini_array_start = .);
  .fini_array     : { KEEP (*(.fini_array)) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  PROVIDE (__fini_array_end = .);
  .ctors          :
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
  } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .dtors          :
  {
    KEEP (*crtbegin*.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend*.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .jcr            : { KEEP (*(.jcr)) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .data.rel.ro : { *(.data.rel.ro.local) *(.data.rel.ro*) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .dynamic        : { *(.dynamic) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .got            : { *(.got.plt) *(.got) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .ramtext        : { *(.ramtext .ramtext.*) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .ddalign	: { . = ALIGN(8); } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    KEEP (*(.gnu.linkonce.d.*personality*))
    SORT(CONSTRUCTORS)
  } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .data1          : { *(.data1) } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  .balign	: { . = ALIGN(8); PROVIDE(_edata = .); } >INTRAM AT>FLASH :INTRAM_AT_FLASH
  PROVIDE (edata = .);
  __bss_start = .;
  .bss            :
  {
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    /* Align here to ensure that the .bss section occupies space up to
       _end.  Align after .bss to ensure correct alignment even if the
       .bss section disappears because there are no input sections.  */
    . = ALIGN(8);
  } >INTRAM AT>INTRAM :INTRAM
  . = ALIGN(8);
  _end = .;
  PROVIDE (end = .);
  __heap_start__ = ALIGN(8);
  .heap           :
  {
    *(.heap)
    . = __heap_size__;
  } >INTRAM AT>INTRAM :INTRAM
  __heap_end__ = .;
  .stack          :
  {
    _stack = .;
    *(.stack)
    . = __stack_size__;
    _estack = .;
  } >INTRAM AT>INTRAM :INTRAM
  _const_lma = LOADADDR(.const);
  .const ALIGN(0x2000) : AT ( LOADADDR(.balign) + SIZEOF (.balign) )
  {
    _const = .;
    *(.evba)
    *(.hash)
    *(.dynsym)
    *(.dynstr)
    *(.gnu.version)
    *(.gnu.version_d)
    *(.gnu.version_r)
    *(.rel.init)
    *(.rela.init)
    *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)
    *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
    *(.rela.fini)
    *(.rel.fini)
    *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
    *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
    *(.rel.data.rel.ro*)
    *(.rela.data.rel.ro*)
    *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
    *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
    *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
    *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
    *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)
    *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
    *(.rel.ctors)
    *(.rela.ctors)
    *(.rel.dtors)
    *(.rela.dtors)
    *(.rel.got)
    *(.rela.got)
    *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)
    *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
    *(.rel.plt)
    *(.rela.plt)
    KEEP (*(.init))
    *(.plt)
    *(.text .stub .text.* .gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    KEEP (*(.fini))
    PROVIDE (__etext = .);
    PROVIDE (_etext = .);
    PROVIDE (etext = .);
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    *(.rodata1)
    *(.eh_frame_hdr)
    KEEP (*(.eh_frame))
    KEEP (*(.gcc_except_table)) *(.gcc_except_table.*)
  } >INTRAM :CONST_INTRAM_AT_FLASH
  . = ALIGN(8);
  _econst = .;
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
  .userpage       : { *(.userpage .userpage.*) } >USERPAGE AT>USERPAGE :USERPAGE
  /DISCARD/ : { *(.note.GNU-stack) }
}
