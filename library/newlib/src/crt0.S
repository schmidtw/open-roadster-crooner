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

#define MAX_PBB_SPEED_MHZ               66

#define SDRAM_START_ADDRESS             (AVR32_EBI_CS1_ADDRESS)
#define SDRAMC_MODE_REGISTER            (AVR32_SDRAMC_ADDRESS+0x00)
#define SDRAMC_REFRESH_TIMER_REGISTER   (AVR32_SDRAMC_ADDRESS+0x04)
#define SDRAMC_CONFIGURATION_REGISTER   (AVR32_SDRAMC_ADDRESS+0x08)
#define SDRAMC_MEMORY_DEVICE_REGISTER   (AVR32_SDRAMC_ADDRESS+0x24)
#define GPIO_PORT_0                     (AVR32_GPIO_ADDRESS+0x0000)
#define GPIO_PORT_1                     (AVR32_GPIO_ADDRESS+0x0100)
#define GPIO_PORT_2                     (AVR32_GPIO_ADDRESS+0x0200)
#define GPIO_PORT_3                     (AVR32_GPIO_ADDRESS+0x0300)
#define GPIO_PORT_4                     (AVR32_GPIO_ADDRESS+0x0400)

/* Memory Chip: MT48LC16M16A2P-7E */
#if   (66 == MAX_PBB_SPEED_MHZ)
#define SDRAMC_CR_VALUE                 (0x531141D9)
#define SDRAMC_TR_VALUE                 (0x203)
#elif (48 == MAX_PBB_SPEED_MHZ)
#define SDRAMC_CR_VALUE                 (0x421131D9)
#define SDRAMC_TR_VALUE                 (0x176)
#elif (33 == MAX_PBB_SPEED_MHZ)
#define SDRAMC_CR_VALUE                 (0x321121D9)
#define SDRAMC_TR_VALUE                 (0x080)
#elif (12 == MAX_PBB_SPEED_MHZ)
#define SDRAMC_CR_VALUE                 (0x111111D9)
#define SDRAMC_TR_VALUE                 (0x05D)
#endif

#define TIME_100uS                      4808


    // This must be linked @ 0x80000000 if it is to be run upon reset without bootloader
    // This must be linked @ 0x80020000 if it is to be run upon reset with bootloader
    .section  .reset, "ax", @progbits


    .global _start
    .type _start, @function
_start:
    // Jump to the C runtime startup routine.
    lda.w   pc, _stext


    // _stext is placed outside the .reset section so that the program entry point
    // can be changed without affecting the C runtime startup.
    .section  .text._stext, "ax", @progbits


    .global _stext
    .type _stext, @function
_stext:

    /* ---------------------------------------------------------------------- */
    /*                              Enable SDRAM                              */
    /* ---------------------------------------------------------------------- */

    // start of with pins driven / needed by EBI
    // Set R0 to GPIO base address Port 1 - pins 63-32
    mov   R0, LO(GPIO_PORT_1)
    orh   R0, HI(GPIO_PORT_1)

    // Clear GPIO control of DRAM pins we need = Enable module control
    mov   R1, LO(0x00017C00)
    orh   R1, HI(0x00017C00)
    st.w  R0[AVR32_GPIO_GPERC], R1  // AVR32_GPIO_GPER Clear
    st.w  R0[AVR32_GPIO_PMR1S], R1  // Put them to mode C/2
    
    // Set R0 to GPIO base address Port 2 - pins 95-64
    mov   R0, LO(GPIO_PORT_2)
    orh   R0, HI(GPIO_PORT_2)

    // Clear GPIO control of DRAM pins we need = Enable module control
    mov   R1, LO(0xFCDBFF40)        // D[5:0], A[17:16], A[14,13], A[10:2], A[0]
    orh   R1, HI(0xFCDBFF40)
    st.w  R0[AVR32_GPIO_GPERC], R1  // AVR32_GPIO_GPER Clear

    // Set R0 to GPIO base address Port 3 - pins 109-96
    mov   R0, LO(GPIO_PORT_3)
    orh   R0, HI(GPIO_PORT_3)

    // Clear GPIO control of DRAM pins we need = Enable module control
    mov   R1, LO(0x000027FF)        // D[6:15],
    orh   R1, HI(0x000027FF)
    st.w  R0[AVR32_GPIO_GPERC], R1  // AVR32_GPIO_GPER Clear

    // Set R0 to Matrix base address - SFR5
    mov   R0, LO(0xFFFE1124)
    orh   R0, HI(0xFFFE1124)

    ld.w  R1, R0
    sbr   R1, 1     // set bit 1  // Enable SDRAM mode for CS1.
    st.w  R0, R1    // Special Function Register 5

    // AVR32_SDRAMC.cr = SDRAMC_CR_VALUE
    mov   R0, LO(SDRAMC_CONFIGURATION_REGISTER)
    orh   R0, HI(SDRAMC_CONFIGURATION_REGISTER)
    mov   R1, LO(SDRAMC_CR_VALUE)
    orh   R1, HI(SDRAMC_CR_VALUE)
    st.w  R0, R1

    //  AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_NOP;
    mov   R0, LO(SDRAMC_MODE_REGISTER)
    orh   R0, HI(SDRAMC_MODE_REGISTER)
    mov   R1, LO(AVR32_SDRAMC_MODE_NOP)
    orh   R1, HI(AVR32_SDRAMC_MODE_NOP)
    st.w  R0, R1

    // write to DRAM to trigger the NOP
    mov   R0, LO(SDRAM_START_ADDRESS)
    orh   R0, HI(SDRAM_START_ADDRESS)
    st.w  R0, R1

    // delay 100u @66Mhz
    mov   R0, LO(TIME_100uS)
    orh   R0, HI(TIME_100uS)
sdram_delay_loop:
    sub   R0, 1
    brne  sdram_delay_loop

    // AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_BANKS_PRECHARGE;
    mov   R0, LO(SDRAMC_MODE_REGISTER)
    orh   R0, HI(SDRAMC_MODE_REGISTER)
    mov   R1, LO(AVR32_SDRAMC_MODE_BANKS_PRECHARGE)
    orh   R1, HI(AVR32_SDRAMC_MODE_BANKS_PRECHARGE)
    st.w  R0, R1

    // write to DRAM to trigger precharge command
    mov   R0, LO(SDRAM_START_ADDRESS)
    orh   R0, HI(SDRAM_START_ADDRESS)
    st.w  R0, R1

    nop // SDRAM_TRP = 15nS

    // AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_AUTO_REFRESH;
    mov   R0, LO(SDRAMC_MODE_REGISTER)
    orh   R0, HI(SDRAMC_MODE_REGISTER)
    mov   R1, LO(AVR32_SDRAMC_MODE_AUTO_REFRESH)
    orh   R1, HI(AVR32_SDRAMC_MODE_AUTO_REFRESH)
    st.w  R0, R1

    // write to DRAM to trigger precharge command
    mov   R0, LO(SDRAM_START_ADDRESS)
    orh   R0, HI(SDRAM_START_ADDRESS)
    st.w  R0, R1

    // SDRAM_TRFC 66nS delay
    nop
    nop
    nop
    mov   R2, LO(8)
    orh   R2, HI(8)

sdram_autorefresh_loop:
    // write to DRAM to trigger precharge command - 66nS between
    st.w  R0, R1
    nop
    nop
    nop
    nop
    sub   R2, 1
    brne  sdram_autorefresh_loop

    //  AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_LOAD_MODE;
    mov   R0, LO(SDRAMC_MODE_REGISTER)
    orh   R0, HI(SDRAMC_MODE_REGISTER)
    mov   R1, LO(AVR32_SDRAMC_MODE_LOAD_MODE)
    orh   R1, HI(AVR32_SDRAMC_MODE_LOAD_MODE)
    st.w  R0, R1

    // write to DRAM to trigger "load mode register" command
    mov   R0, LO(SDRAM_START_ADDRESS)
    orh   R0, HI(SDRAM_START_ADDRESS)
    st.w  R0, R1

    // SDRAM_TMRD
    nop
    nop

    // Switch the SDRAM Controller to normal mode.
    mov   R0, LO(SDRAMC_MODE_REGISTER)
    orh   R0, HI(SDRAMC_MODE_REGISTER)
    mov   R1, LO(AVR32_SDRAMC_MODE_NORMAL)
    orh   R1, HI(AVR32_SDRAMC_MODE_NORMAL)
    st.w  R0, R1

    // write to DRAM to trigger "Normal mode" command
    mov   R0, LO(SDRAM_START_ADDRESS)
    orh   R0, HI(SDRAM_START_ADDRESS)
    st.w  R0, R1

    // AVR32_SDRAMC.tr = SDRAMC_TR_VALUE
    mov   R0, LO(SDRAMC_REFRESH_TIMER_REGISTER)
    orh   R0, HI(SDRAMC_REFRESH_TIMER_REGISTER)
    mov   R1, LO(SDRAMC_TR_VALUE)
    orh   R1, HI(SDRAMC_TR_VALUE)
    st.w  R0, R1

    /* ---------------------------------------------------------------------- */
    /*                              Enable SDRAM                              */
    /* ---------------------------------------------------------------------- */

    /* Zero marks the end of the outermost frame.  */
    mov   r7, 0

    /*  Set up the stack pointer to a fixed value */
    lda.w r10, _estack

    /*  If _stack is -1 don't set sp, the stack is then probably
        set by some other means i.e. by a bootloader. */
    cp    r10, -1
    movne sp, r10

    /* Copy the data segment into RAM if necessary. If data_lma
       is equal to data then VMA==LMA and no copying is necessary.
       The linker script must make sure that the data segment is at
       least dword-aligned.  */
    lda.w r11, _data_lma
    lda.w r12, _data
    cp.w  r11, r12
    breq  1f
    lda.w r10, _edata
2:  ld.d  r8, r11++
    st.d  r12++, r8
    cp.w  r12, r10
    brlt  2b
    
    /* Zero the memory in the .bss section.  */
1:  lda.w r10, _end
    lda.w r12, _edata

    mov   r8, 0
    mov   r9, 0
2:  st.d  r12++, r8
    cp.w  r12, r10
    brlt  2b

    /* User dependent initialization */
    call  _init_startup

    /*Initialize argv, use stack for keeping arguments. 
          _init_argv() returns argc in r12 and argv in r11
     and the total size used for the arguments in r10.*/
    mov   r12, sp 
    /* Signal that we are storing the arguments in a stackwise
       top down approach. */    
    mov   r11, 0  
    call  _init_argv
    /* If initialization if argv is not handled then _init_argv
           returns -1 so set argc to 0 and make sure no space is 
       allocated on the stack. */
    cp.w  r12, -1
    moveq r12, 0
    moveq r10, 0
    sub   sp, r10

    /* Call main and call exit when it returns */
    call  main
    call  exit

    .weak _data_lma
