/*
 * Copyright (c) 2011  Weston Schmidt
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

#include <stdint.h>

#include "reboot.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* Needed 'externally' by other BSP Functions */
reboot_trace_t *reboot_pointer_get( void );
uint32_t reboot_calculate_checksum( reboot_trace_t *trace );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
reboot_trace_t *reboot_get_last( void )
{
    uint32_t checksum;
    reboot_trace_t *trace;

    trace = reboot_pointer_get();

    checksum = reboot_calculate_checksum( trace );

    if( checksum == trace->checksum ) {
        return trace;
    }

    return NULL;
}

void reboot_output( FILE *out, reboot_trace_t *trace )
{
    int i;

    if( (NULL == out) || (NULL == trace) ) {
        return;
    }

    fprintf( out, "Processor Info (0x%08lx)", trace->config0 );
    fprintf( out, ", id=0x%08lx", (trace->config0 >> 24) );
    fprintf( out, ", proc rev=%ld", (0x03 & (trace->config0 >> 16)) );
    fprintf( out, ", arch=%s", ((0x03 & (trace->config0 >> 13)) == 0) ? "AVR32A" : 
                               ((0x03 & (trace->config0 >> 13)) == 1) ? "AVR32B" : "Unkown" );
    fprintf( out, ", arch rev=%ld", (0x03 & (trace->config0 >> 10)) );
    fprintf( out, ", mmut=%ld", (0x03 & (trace->config0 >> 7)) );
    fprintf( out, ", fpu=%s", (0x01 & (trace->config0 >> 6)) ? "yes" : "no" );
    fprintf( out, ", java=%s", (0x01 & (trace->config0 >> 5)) ? "yes" : "no" );
    fprintf( out, ", perf counters=%s", (0x01 & (trace->config0 >> 4)) ? "yes" : "no" );
    fprintf( out, ", ocd=%s", (0x01 & (trace->config0 >> 3)) ? "yes" : "no" );
    fprintf( out, ", simd=%s", (0x01 & (trace->config0 >> 2)) ? "yes" : "no" );
    fprintf( out, ", dsp=%s", (0x01 & (trace->config0 >> 1)) ? "yes" : "no" );
    fprintf( out, ", rmw=%s", (0x01 & trace->config0) ? "yes" : "no" );
    fprintf( out, "\n" );
    fprintf( out, "CPU Cycle Count: %lu\n", trace->cpu_count );

    fprintf( out, "Exception: " );
    switch( trace->exception_cause ) {
        case 0x00: fprintf( out, "Unrecoverable exception\n" );                     break;
        case 0x04: fprintf( out, "TLB multiple hit\n" );                            break;
        case 0x08: fprintf( out, "Bus error data fetch 0x%08lx\n", trace->bear );   break;
        case 0x0c: fprintf( out, "Bus error instruction fetch\n" );                 break;
        case 0x10: fprintf( out, "Non maskible interrupt\n" );                      break;
        case 0x14: fprintf( out, "Instruction address\n" );                         break;
        case 0x18: fprintf( out, "ITLB protection\n" );                             break;
        case 0x1c: fprintf( out, "Breakpoint\n" );                                  break;
        case 0x20: fprintf( out, "Illegal opcode\n" );                              break;
        case 0x24: fprintf( out, "Unimplemented instruction\n" );                   break;
        case 0x28: fprintf( out, "Privilege violation\n" );                         break;
        case 0x2c: fprintf( out, "Floating point\n" );                              break;
        case 0x30: fprintf( out, "Coprossor absent\n" );                            break;
        case 0x34: fprintf( out, "Data address (read)\n" );                         break;
        case 0x38: fprintf( out, "Data address (write)\n" );                        break;
        case 0x3c: fprintf( out, "DTLB protection (read)\n" );                      break;
        case 0x40: fprintf( out, "DTLB protection (write)\n" );                     break;
        case 0x44: fprintf( out, "DTLB modified\n" );                               break;
        case 0x50: fprintf( out, "ITLB miss\n" );                                   break;
        case 0x60: fprintf( out, "DTLB miss (read)\n" );                            break;
        case 0x70: fprintf( out, "DTLB miss (write)\n" );                           break;
        default:   fprintf( out, "Unknown: '0x%08lx'\n", trace->exception_cause );  break;
    }

    switch( trace->exception_cause ) {
        case 0x08:
        case 0x0c:
        case 0x10:
            fprintf( out, "First non-issued instruction: 0x%08lx\n", trace->return_address );
            break;

        default:
            fprintf( out, "Offending instruction: 0x%08lx\n", trace->return_address );
    }

    fprintf( out, "-- Register File ---------------\n" );
    for( i = 0; i < 10; i++ ) {
        fprintf( out, " r%d: 0x%08lx\n", i, trace->r[i] );
    }
    fprintf( out, "r10: 0x%08lx\n", trace->r[10] );
    fprintf( out, "r11: 0x%08lx\n", trace->r[11] );
    fprintf( out, "r12: 0x%08lx\n", trace->r[12] );

    fprintf( out, " sp: 0x%08lx\n", trace->r[13] );
    fprintf( out, " lr: 0x%08lx\n", trace->r[14] );
    fprintf( out, " pc: 0x%08lx\n", trace->r[15] );
    fprintf( out, " sr: 0x%08lx\n", trace->sr );
    fprintf( out, "--------------------------------\n" );
    fprintf( out, "-- Stack Trace Top -> Bottom ---\n" );
    for( i = 0; i < MAX_STACK_DEPTH; i++ ) {
        if( 0 != trace->stack[i] ) {
            fprintf( out, "0x%08lx\n", trace->stack[i] );
        }
    }
    fprintf( out, "--------------------------------\n" );
}

reboot_trace_t *reboot_pointer_get( void )
{
    extern void __reset_log__;
    return (reboot_trace_t *) &__reset_log__;
}

uint32_t reboot_calculate_checksum( reboot_trace_t *trace )
{
    uint32_t checksum, i;

    checksum = 0;
    checksum ^= trace->cpu_count;
    checksum ^= trace->bear;
    checksum ^= trace->config0;
    checksum ^= trace->exception_cause;
    checksum ^= trace->return_address;
    checksum ^= trace->sr;
    for( i = 0; i < 16; i++ ) {
        checksum ^= trace->r[i];
        checksum ^= trace->stack[i];
    }

    return checksum;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
