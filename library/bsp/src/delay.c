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
#include <stdio.h>
#include <stdint.h>

#include <avr32/io.h>

#include "delay.h"
#include "pm.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define ONE_SECOND  1000000000UL

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

/* See delay.h for details */
void delay_cycles( const uint32_t cycles )
{
    uint32_t i;

    for( i = cycles; 0 < i; i-- ) {
        asm( "nop" );
    }
}

/* See delay.h for details */
void delay_time( const uint64_t ns )
{
    uint64_t left;
    uint32_t clock;

    left = ns;

    clock = pm_get_frequency( PM__CPU );

    while( ONE_SECOND < left ) {
        delay_cycles( clock );
        left -= ONE_SECOND;
    }

    if( 0 < left ) {
        left = ((left * clock) + 999999999UL) / 1000000000UL;
        delay_cycles( (uint32_t) left );
        left = 0;
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
