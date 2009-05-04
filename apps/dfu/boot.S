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


#include <avr32/io.h>
#include "conf_isp.h"


  // Performs efficiently a bitwise logical Exclusive-OR between the specified
  // register and an immediate value of up to 32 bits. The result is stored in
  // the destination register.
  .macro  eor.w   rd, imm
    .if \imm & 0x0000FFFF
      eorl    \rd, LO(\imm)
    .endif
    .if \imm & 0xFFFF0000
      eorh    \rd, HI(\imm)
    .endif
  .endm

  // Moves efficiently an immediate value of up to 32 bits into a register.
  .macro  mov.w   rd, imm
    .if ((-(1 << (21 - 1))) <= \imm) && (\imm <= ((1 << (21 - 1)) - 1))
      mov     \rd, \imm
#if __AVR32_UC__ >= 2
    .elseif !(\imm & 0x0000FFFF)
      movh    \rd, HI(\imm)
#endif
    .else
      mov     \rd, LO(\imm)
      orh     \rd, HI(\imm)
    .endif
  .endm

  // Performs efficiently a bitwise logical OR between the specified register
  // and an immediate value of up to 32 bits. The result is stored in the
  // destination register.
  .macro  or.w    rd, imm
    .if \imm & 0x0000FFFF
      orl     \rd, LO(\imm)
    .endif
    .if \imm & 0xFFFF0000
      orh     \rd, HI(\imm)
    .endif
  .endm


  .section  .reset, "ax", @progbits


  .balign 2

  // Reset vector: This must be linked @ 0x80000000.
  .global _start
  .type _start, @function
_start:
  mov.w   r9, AVR32_WDT_ADDRESS
  mov.w   r10, AVR32_FLASHC_ADDRESS
  mov.w   r11, AVR32_PM_ADDRESS
  ld.w    r2, r10[AVR32_FLASHC_FGPFRLO]
  ld.w    r3, r11[AVR32_PM_RCAUSE]
  mov.w   r4, AVR32_PM_RCAUSE_POR_MASK |\
              AVR32_PM_RCAUSE_EXT_MASK |\
              AVR32_PM_RCAUSE_JTAG_MASK |\
              AVR32_PM_RCAUSE_OCDRST_MASK |\
              AVR32_PM_RCAUSE_JTAGHARD_MASK
  tst     r3, r4
  brne    manage_io_cond
  bld     r3, AVR32_PM_RCAUSE_WDT_OFFSET
  brcs    start_program

start_loader:
  rcall   disable_wdt

  // Set initial stack pointer.
  mov     sp, _estack

  // Disable the exception processing.
  ssrf    AVR32_SR_EM_OFFSET

  // Set up EVBA so interrupts can be enabled.
  mov     r0, _evba
  mtsr    AVR32_EVBA, r0

  // Load initialized data having a global lifetime from the data LMA.
  mov     r0, _data
  mov     r1, _edata
  sub     r2, pc, $ - _data_lma
  rcall   load_idata

  // Clear uninitialized data having a global lifetime in the blank static storage section.
  mov     r0, __bss_start
  mov     r1, _end
  mov.w   r2, 0
  mov.w   r3, 0
  rjmp    clear_udata
clear_udata_loop:
  st.d    r0++, r2
clear_udata:
  cp.w    r0, r1
  brlo    clear_udata_loop

  // Load constant data and code from the const LMA.
  mov     r0, _const
  mov     r1, _econst
  sub     r2, pc, $ - _const_lma
  rcall   load_idata

  // Call the ISP main function, which must not return.
  call    main

manage_io_cond:
  mov.w   r3, AVR32_PIN_PB01
  mov.w   r10, AVR32_GPIO_ADDRESS
  lsr     r4, r3, 5
  lsl     r4, 8
  add     r10, r4
  ld.w    r4, r10[AVR32_GPIO_PVR]
  andl    r3, 0x1F
  lsr     r4, r4, r3
  mov.w   r3, 1
  eor     r4, r3
  bld     r4, 0
  brcc    start_loader

start_program:
  rcall   disable_wdt
  .global boot_program
  .type boot_program, @function
boot_program:
  st.w    r8[0], r0
start_program_no_isp_key:
  mov.w   r0, AVR32_SR_GM_MASK | AVR32_SR_EM_MASK | (AVR32_SR_M_SUP << AVR32_SR_M_OFFSET)
  mtsr    AVR32_SR, r0
  .irp    rd, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr
    mov.w   \rd, 0
  .endr
  mtsr    AVR32_EVBA, r0
  mtsr    AVR32_COUNT, r0
  lddpc   pc, program_start_address

disable_wdt:
  mov.w   r2, AVR32_WDT_KEY_VALUE << AVR32_WDT_CTRL_KEY_OFFSET
  st.w    r9[AVR32_WDT_CTRL], r2
  eor.w   r2, AVR32_WDT_CTRL_KEY_MASK
  st.w    r9[AVR32_WDT_CTRL], r2
  mov     pc, lr

load_idata_loop:
  ld.d    r4, r2++
  st.d    r0++, r4
load_idata:
  cp.w    r0, r1
  brlo    load_idata_loop
  mov     pc, lr


// Constant data area.

  .balign 4

program_start_address:
  .word PROGRAM_START_ADDRESS


  .section  .evba, "ax", @progbits


  .balign 2

  // Start of exception vector table: Unrecoverable exception.
  .global _evba
  .type _evba, @function
_evba:
  lda.w   r8, _start
  mov.w   r9, AVR32_SR_GM_MASK | AVR32_SR_EM_MASK | (AVR32_SR_M_SUP << AVR32_SR_M_OFFSET)
  mov     sp, _estack - 6 * 4
  pushm   r8-r9
  rete
