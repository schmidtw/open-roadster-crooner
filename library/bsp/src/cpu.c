/*
 * Copyright (c) 2009  Weston Schmidt
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

#include "boards/boards.h"
#include "cpu.h"
#include "wdt.h"
#include "intc.h"

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
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See cpu.h for details. */
cpu_execution_mode_t cpu_get_mode( void )
{
    int sr;
    
    sr = __builtin_mustr();

    sr &= AVR32_SR_M0_MASK | AVR32_SR_M1_MASK | AVR32_SR_M2_MASK;
    sr >>= AVR32_SR_M0_OFFSET;

    return (cpu_execution_mode_t) sr;
}

/* See cpu.h for details. */
void cpu_reboot( void )
{
    disable_global_interrupts();

    wdt_start( WDT__1ms );

    while( 1 ) { ; }
}

/* See cpu.h for details. */
void cpu_disable_orphans( void )
{
#ifdef DOSM_CPU_MASK
    AVR32_PM.cpumask = DOSM_CPU_MASK;
#endif
#ifdef DOSM_HSB_MASK
    AVR32_PM.hsbmask = DOSM_HSB_MASK;
#endif
#ifdef DOSM_PBA_MASK
    AVR32_PM.pbamask = DOSM_PBA_MASK;
#endif
#ifdef DOSM_PBB_MASK
    AVR32_PM.pbbmask = DOSM_PBB_MASK;
#endif
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
