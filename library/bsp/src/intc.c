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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/reent.h>

#include <avr32/io.h>

#include "boards/boards.h"
#include "intc.h"
#include "usart.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    uint32_t irq_count;
    volatile intc_handler_t *table;
} __intc_handler_table_t;

/*----------------------------------------------------------------------------*/
/*                             External Variables                             */
/*----------------------------------------------------------------------------*/
/* Defined in exception.S */
extern const uint32_t isr_val[4];

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile intc_handler_t __intc_line_handler__0[AVR32_INTC_NUM_IRQS_PER_GRP0];
static volatile intc_handler_t __intc_line_handler__1[AVR32_INTC_NUM_IRQS_PER_GRP1];
static volatile intc_handler_t __intc_line_handler__2[AVR32_INTC_NUM_IRQS_PER_GRP2];
static volatile intc_handler_t __intc_line_handler__3[AVR32_INTC_NUM_IRQS_PER_GRP3];
static volatile intc_handler_t __intc_line_handler__4[AVR32_INTC_NUM_IRQS_PER_GRP4];
static volatile intc_handler_t __intc_line_handler__5[AVR32_INTC_NUM_IRQS_PER_GRP5];
static volatile intc_handler_t __intc_line_handler__6[AVR32_INTC_NUM_IRQS_PER_GRP6];
static volatile intc_handler_t __intc_line_handler__7[AVR32_INTC_NUM_IRQS_PER_GRP7];
static volatile intc_handler_t __intc_line_handler__8[AVR32_INTC_NUM_IRQS_PER_GRP8];
static volatile intc_handler_t __intc_line_handler__9[AVR32_INTC_NUM_IRQS_PER_GRP9];
static volatile intc_handler_t __intc_line_handler__10[AVR32_INTC_NUM_IRQS_PER_GRP10];
static volatile intc_handler_t __intc_line_handler__11[AVR32_INTC_NUM_IRQS_PER_GRP11];
static volatile intc_handler_t __intc_line_handler__12[AVR32_INTC_NUM_IRQS_PER_GRP12];
static volatile intc_handler_t __intc_line_handler__13[AVR32_INTC_NUM_IRQS_PER_GRP13];
static volatile intc_handler_t __intc_line_handler__14[AVR32_INTC_NUM_IRQS_PER_GRP14];
static volatile intc_handler_t __intc_line_handler__15[AVR32_INTC_NUM_IRQS_PER_GRP15];
static volatile intc_handler_t __intc_line_handler__16[AVR32_INTC_NUM_IRQS_PER_GRP16];
static volatile intc_handler_t __intc_line_handler__17[AVR32_INTC_NUM_IRQS_PER_GRP17];
#ifdef AVR32_INTC_NUM_IRQS_PER_GRP18
static volatile intc_handler_t __intc_line_handler__18[AVR32_INTC_NUM_IRQS_PER_GRP18];
#endif
#ifdef AVR32_INTC_NUM_IRQS_PER_GRP19
static volatile intc_handler_t __intc_line_handler__19[AVR32_INTC_NUM_IRQS_PER_GRP19];
#endif

static const __intc_handler_table_t __intc_handler_table[] = {
    { AVR32_INTC_NUM_IRQS_PER_GRP0,  __intc_line_handler__0  },
    { AVR32_INTC_NUM_IRQS_PER_GRP1,  __intc_line_handler__1  },
    { AVR32_INTC_NUM_IRQS_PER_GRP2,  __intc_line_handler__2  },
    { AVR32_INTC_NUM_IRQS_PER_GRP3,  __intc_line_handler__3  },
    { AVR32_INTC_NUM_IRQS_PER_GRP4,  __intc_line_handler__4  },
    { AVR32_INTC_NUM_IRQS_PER_GRP5,  __intc_line_handler__5  },
    { AVR32_INTC_NUM_IRQS_PER_GRP6,  __intc_line_handler__6  },
    { AVR32_INTC_NUM_IRQS_PER_GRP7,  __intc_line_handler__7  },
    { AVR32_INTC_NUM_IRQS_PER_GRP8,  __intc_line_handler__8  },
    { AVR32_INTC_NUM_IRQS_PER_GRP9,  __intc_line_handler__9  },
    { AVR32_INTC_NUM_IRQS_PER_GRP10, __intc_line_handler__10 },
    { AVR32_INTC_NUM_IRQS_PER_GRP11, __intc_line_handler__11 },
    { AVR32_INTC_NUM_IRQS_PER_GRP12, __intc_line_handler__12 },
    { AVR32_INTC_NUM_IRQS_PER_GRP13, __intc_line_handler__13 },
    { AVR32_INTC_NUM_IRQS_PER_GRP14, __intc_line_handler__14 },
    { AVR32_INTC_NUM_IRQS_PER_GRP15, __intc_line_handler__15 },
    { AVR32_INTC_NUM_IRQS_PER_GRP16, __intc_line_handler__16 },
    { AVR32_INTC_NUM_IRQS_PER_GRP17, __intc_line_handler__17 }
#ifdef AVR32_INTC_NUM_IRQS_PER_GRP18
   ,{ AVR32_INTC_NUM_IRQS_PER_GRP18, __intc_line_handler__18 }
#endif
#ifdef AVR32_INTC_NUM_IRQS_PER_GRP19
   ,{ AVR32_INTC_NUM_IRQS_PER_GRP19, __intc_line_handler__19 }
#endif
};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __intc_init( void );
__attribute__ ((__interrupt__))
static void __unhandled_interrupt( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See intc.h for details. */
inline void disable_global_interrupts( void )
{
    asm( "ssrf %0" :  : "i" (AVR32_SR_GM_OFFSET) );
}

/* See intc.h for details. */
inline void enable_global_interrupts( void )
{
    asm( "csrf %0" :  : "i" (AVR32_SR_GM_OFFSET) );
}

/* See intc.h for details. */
inline bool are_global_interrupts_enabled( void )
{
    return (bool) (!(__builtin_mfsr(AVR32_SR) & AVR32_SR_GM_MASK));
}

/* See intc.h for details. */
inline void disable_global_exceptions( void )
{
    asm( "ssrf %0" :  : "i" (AVR32_SR_EM_OFFSET) );
}

/* See intc.h for details. */
inline void enable_global_exceptions( void )
{
    asm( "csrf %0" :  : "i" (AVR32_SR_EM_OFFSET) );
}

/* See intc.h for details. */
inline bool are_global_exceptions_enabled( void )
{
    return (bool) (!(__builtin_mfsr(AVR32_SR) & AVR32_SR_EM_MASK));
}


/* See intc.h for details. */
inline bool interrupts_save_and_disable( void )
{
    if( true == are_global_interrupts_enabled() ) {
        disable_global_interrupts();
        return true;
    }
    return false;
}

/* See intc.h for details. */
inline void interrupts_restore( const bool state )
{
    if( true == state ) {
        enable_global_interrupts();
    }
}

/* See intc.h for details. */
bsp_status_t intc_register_isr( const intc_handler_t handler,
                                const intc_isr_t isr,
                                const intc_level_t level )
{
    uint32_t group, line;

    __intc_init();

    group = ((uint32_t) isr) >> 8;
    line = 0x00ff & ((uint32_t) isr);

    if( sizeof(__intc_handler_table)/sizeof(__intc_handler_table_t) <= group ) {
        return BSP_ERROR_PARAMETER;
    }


    if( __intc_handler_table[group].irq_count <= line ) {
        return BSP_ERROR_PARAMETER;
    }

    if( 3 < (uint32_t) level ) {
        return BSP_ERROR_PARAMETER;
    }

    if( NULL == handler ) {
        __intc_handler_table[group].table[line] = &__unhandled_interrupt;
        AVR32_INTC.ipr[group] = isr_val[0]; /* default */
    } else {
        __intc_handler_table[group].table[line] = handler;
        AVR32_INTC.ipr[group] = isr_val[level];
    }

    return BSP_RETURN_OK;
}

/* See intc.h for details. */
void intc_isr_puts( const char *out )
{
    int i;
    for( i = 0; 0 != out[i]; i++ ) {
        while( false == usart_tx_ready(DEBUG_USART) ) { ; }

        usart_write_char( DEBUG_USART, out[i] );
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to initialize the interrupt handling code.
 */
void __intc_init( void )
{
    static uint8_t initialized = 0;

    if( 0x2f != initialized ) {
        int32_t group, req;

        for( group = 0; group < AVR32_INTC_NUM_INT_GRPS; group++ ) {
            for( req = 0; req < __intc_handler_table[group].irq_count; req++ ) {
                __intc_handler_table[group].table[req] = &__unhandled_interrupt;
            }

            /* Default to level 0 */
            AVR32_INTC.ipr[group] = isr_val[0];
        }

        /* System has been initialized. */
        initialized = 0x2f;
    }
}

/**
 *  Used to handle all unhandled interrupts & halt the CPU.
 */
__attribute__ ((__interrupt__))
static void __unhandled_interrupt( void )
{
    struct _reent reent;

    _REENT_INIT_PTR( (&(reent)) );
    _impure_ptr = &reent;

    fprintf( stderr, "\nunhandled interrupt\n" );
    fflush( stderr );
    while( 1 ) { ; }
}

/**
 *  Used to handle the special SCALL used by FreeRTOS, but if we don't
 *  link against it, make sure the linkage works.
 *
 *  @note This is externally visible only because it has to be, & should
 *        never be directly called except through exception.S.
 */
__attribute__ ((weak))
void SCALLYield( void )
{
    while( 1 ) { ; }
}

/**
 *  Used by exception.S to find the handler to call.
 *
 *  @note Needs to not be static so it is linked in.  Should never
 *        directly be called except through exception.S.
 *
 *  @param level the interrupt level to handle
 *
 *  @return the handler to call, or NULL to skip
 */
intc_handler_t __bsp_interrupt_handler( uint32_t index )
{
    uint32_t group, req;

    /* The list is reversed - so we need to invert the index as well. */
    group = AVR32_INTC.icr[index];
    req = AVR32_INTC.irr[group];

    if( 0 == req ) {
        return NULL;
    }

    /* Find the group, then determine the most important interrupt to run
     * by using the clz to find the first '1' bit (from MSB to LSB), lookup
     * the function & return. */
    req = 31 - __builtin_clz( req );

    return __intc_handler_table[group].table[req];
}

/**
 *  A helper function that prints out some useful information
 *  when we get an exception of some sort.
 *
 *  @note Needs to not be static so it is linked in.  Should never
 *        directly be called except through exception.S.
 *
 *  @param type the different exceptions defined in exceptions.S
 */
void __bsp_exception_handler( uint32_t exception_cause, uint32_t return_address, uint32_t *sp )
{
    uint32_t bear = __builtin_mfsr( AVR32_BEAR );

    static struct _reent reent;

    _REENT_INIT_PTR( (&(reent)) );
    _impure_ptr = &reent;

    fprintf( stderr, "\nException: " );
    switch( exception_cause ) {
        case 0x00: fprintf( stderr, "Unrecoverable exception\n" );               break;
        case 0x04: fprintf( stderr, "TLB multiple hit\n" );                      break;
        case 0x08: fprintf( stderr, "Bus error data fetch 0x%08lx\n", bear );    break;
        case 0x0c: fprintf( stderr, "Bus error instruction fetch\n" );           break;
        case 0x10: fprintf( stderr, "Non maskible interrupt\n" );                break;
        case 0x14: fprintf( stderr, "Instruction address\n" );                   break;
        case 0x18: fprintf( stderr, "ITLB protection\n" );                       break;
        case 0x1c: fprintf( stderr, "Breakpoint\n" );                            break;
        case 0x20: fprintf( stderr, "Illegal opcode\n" );                        break;
        case 0x24: fprintf( stderr, "Unimplemented instruction\n" );             break;
        case 0x28: fprintf( stderr, "Privilege violation\n" );                   break;
        case 0x2c: fprintf( stderr, "Floating point\n" );                        break;
        case 0x30: fprintf( stderr, "Copressor absent\n" );                      break;
        case 0x34: fprintf( stderr, "Data address (read)\n" );                   break;
        case 0x38: fprintf( stderr, "Data address (write)\n" );                  break;
        case 0x3c: fprintf( stderr, "DTLB protection (read)\n" );                break;
        case 0x40: fprintf( stderr, "DTLB protection (write)\n" );               break;
        case 0x44: fprintf( stderr, "DTLB modified\n" );                         break;
        case 0x50: fprintf( stderr, "ITLB miss\n" );                             break;
        case 0x60: fprintf( stderr, "DTLB miss (read)\n" );                      break;
        case 0x70: fprintf( stderr, "DTLB miss (write)\n" );                     break;
        default:   fprintf( stderr, "Unknown '0x%08lx'\n", exception_cause );    break;
    }

    switch( exception_cause ) {
        case 0x08:
        case 0x0c:
        case 0x10:
            fprintf( stderr, "First non-issued instruction: 0x%08lx\n", return_address );
            break;

        default:
            fprintf( stderr, "Offending instruction: 0x%08lx\n", return_address );
    }

    /* Walk the stack and print out any addresses that are in code space. */
    while( 0xdeadbeef != *sp ) {
        if( 0x80000000 == (0xF0000000 & *sp) ) {
            fprintf( stderr, "0x%08lx\n", *sp );
        }
        sp++;
    }

    fflush( stderr );
    fflush( stdout );

    while( 1 ) { ; }
}
