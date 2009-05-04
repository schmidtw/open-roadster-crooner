/*
 * Copyright (c) 2008  Weston Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <avr32/io.h>

        .section        .exception, "ax"
        .align  9
        .global _evba
_evba:
        .org    0x000                       // Unrecoverable exception
        rjmp    _handle_exception
        .org    0x004                       // TLB multiple hit
        rjmp    _handle_exception
        .org    0x008                       // Bus error data fetch
        rjmp    _handle_exception
        .org    0x00c                       // Bus error instruction fetch
        rjmp    _handle_exception
        .org    0x010                       // Non Maskible Interrupt
        rjmp    _handle_exception
        .org    0x014                       // Instruction Address
        rjmp    _handle_exception
        .org    0x018                       // ITLB Protection
        rjmp    _handle_exception
        .org    0x01c                       // Breakpoint
        rjmp    _handle_exception
        .org    0x020                       // Illegal Opcode
        rjmp    _handle_exception
        .org    0x024                       // Unimplemented instruction
        rjmp    _handle_exception
        .org    0x028                       // Privilege violation
        rjmp    _handle_exception
        .org    0x02c                       // Floating point
        rjmp    _handle_exception
        .org    0x030                       // Coprocessor absent
        rjmp    _handle_exception
        .org    0x034                       // Data Address (Read)
        rjmp    _handle_exception
        .org    0x038                       // Data Address (Write)
        rjmp    _handle_exception
        .org    0x03c                       // DTLB Protection (Read)
        rjmp    _handle_exception
        .org    0x040                       // DTLB Protection (Write)
        rjmp    _handle_exception
        .org    0x044                       // DTLB Modified
        rjmp    _handle_exception
        .org    0x050                       // ITLB Miss
        rjmp    _handle_exception
        .org    0x060                       // DTLB Miss (Read)
        rjmp    _handle_exception
        .org    0x070                       // DTLB Miss (Write)
        rjmp    _handle_exception
        .org    0x0100                      // Supervisor call
        lda.w   pc, SCALLYield

        .align  2

.irp    priority,0,1,2,3
_int\priority:
        // R8 - R12, LR, PC and SR are automatically pushed to the stack for UC3
        // devices.

        // Stack:
        //   R8  -7
        //   R9  -6
        //   R10 -5
        //   R11 -4
        //   R12 -3
        //   LR  -2
        //   PC  -1
        //   SR  -0

        mov     r12, 3 - \priority          // pass index to the _interrupt_handler function
        call    __bsp_interrupt_handler
        cp.w    r12, 0                      // Check the return value for 'NULL'
        movne   pc, r12                     // Jump to the handler if not NULL, return otherwise
        rete
.endr

        .global isr_val
        .type isr_val, @object

isr_val:
        .word   (AVR32_INTC_INT0 << AVR32_INTC_IPR_INTLEVEL_OFFSET) | (_int0 - _evba),  \
                (AVR32_INTC_INT1 << AVR32_INTC_IPR_INTLEVEL_OFFSET) | (_int1 - _evba),  \
                (AVR32_INTC_INT2 << AVR32_INTC_IPR_INTLEVEL_OFFSET) | (_int2 - _evba),  \
                (AVR32_INTC_INT3 << AVR32_INTC_IPR_INTLEVEL_OFFSET) | (_int3 - _evba)

_handle_exception:
        mfsr    r12, AVR32_ECR
        lsl     r12, 2
        lddsp   r11, sp[4]
        call    __bsp_exception_handler
        rjmp    $
