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
#include "cpu.h"
#include "intc.h"
#include "reboot.h"
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

extern reboot_trace_t *reboot_pointer_get( void );
extern uint32_t reboot_calculate_checksum( reboot_trace_t *trace );


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See intc.h for details. */
void disable_global_interrupts( void )
{
    asm( "ssrf %0" :  : "i" (AVR32_SR_GM_OFFSET) );
}

/* See intc.h for details. */
void enable_global_interrupts( void )
{
    asm( "csrf %0" :  : "i" (AVR32_SR_GM_OFFSET) );
}

/* See intc.h for details. */
bool are_global_interrupts_enabled( void )
{
    return (bool) (!(__builtin_mfsr(AVR32_SR) & AVR32_SR_GM_MASK));
}

/* See intc.h for details. */
void disable_global_exceptions( void )
{
    asm( "ssrf %0" :  : "i" (AVR32_SR_EM_OFFSET) );
}

/* See intc.h for details. */
void enable_global_exceptions( void )
{
    asm( "csrf %0" :  : "i" (AVR32_SR_EM_OFFSET) );
}

/* See intc.h for details. */
bool are_global_exceptions_enabled( void )
{
    return (bool) (!(__builtin_mfsr(AVR32_SR) & AVR32_SR_EM_MASK));
}


/* See intc.h for details. */
bool interrupts_save_and_disable( void )
{
    if( true == are_global_interrupts_enabled() ) {
        disable_global_interrupts();
        return true;
    }
    return false;
}

/* See intc.h for details. */
void interrupts_restore( const bool state )
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

    if( NULL == handler ) {
        return BSP_ERROR_PARAMETER;
    }

    if( sizeof(__intc_handler_table)/sizeof(__intc_handler_table_t) <= group ) {
        return BSP_ERROR_PARAMETER;
    }


    if( __intc_handler_table[group].irq_count <= line ) {
        return BSP_ERROR_PARAMETER;
    }

    if( 3 < (uint32_t) level ) {
        return BSP_ERROR_PARAMETER;
    }

    __intc_handler_table[group].table[line] = handler;
    AVR32_INTC.ipr[group] = isr_val[level];

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
    cpu_reboot();
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
    intc_handler_t fn;

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

    fn = __intc_handler_table[group].table[req];

    if( __unhandled_interrupt != fn ) {
        return fn;
    } else {
        static reboot_trace_t *trace;

        trace = reboot_pointer_get();

        memset( trace, 0, sizeof(reboot_trace_t) );

        trace->cpu_count = cpu_get_sys_count();
        trace->exception_cause = 0x1000;
        trace->r[0] = group;
        trace->r[1] = req;
  
        trace->checksum = reboot_calculate_checksum( trace );

        cpu_reboot();
    }

    return NULL;
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
void __bsp_exception_handler( uint32_t exception_cause, uint32_t return_address,
                              uint32_t *sp, uint32_t *r, uint32_t sr )
{
    static reboot_trace_t *trace;
    static int i;

    trace = reboot_pointer_get();

    trace->cpu_count = cpu_get_sys_count();
    trace->exception_cause = exception_cause;
    trace->return_address = return_address;
    trace->bear = __builtin_mfsr( AVR32_BEAR );
    trace->config0 = __builtin_mfsr( AVR32_CONFIG0 );

    memset( trace->firmware, '\0', 0x40 );
    /* Ideally the 0x80002200 comes from the linker script. */
    strcpy( trace->firmware, (char*) 0x80002200 );

    for( i = 0; i < 16; i++ ) {
        trace->r[i] = r[15 - i];
    }
    trace->r[13] = (uint32_t) sp;
    trace->sr = sr;

    i = 0;
    /* Walk the stack and print out any addresses that are in code space. */
    while( 0xdeadbeef != *sp ) {
        /* Ideally these come from the linker script... but they're bad. */
        if( (0x80002200 <= *sp) && (*sp <= 0x80080000) ) {
            if( i < MAX_STACK_DEPTH ) {
                trace->stack[i++] = *sp;
            }
        }
        sp++;
    }

    /* Zero out any unused values */
    while( i < MAX_STACK_DEPTH ) {
        trace->stack[i++] = 0;
    }

    trace->checksum = reboot_calculate_checksum( trace );

    cpu_reboot();
}
