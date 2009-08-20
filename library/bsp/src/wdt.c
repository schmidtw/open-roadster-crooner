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

#include "wdt.h"

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
/* See wdt.h for details. */
void wdt_start( const wdt_timeout_t timeout )
{
    avr32_wdt_t wdt;

    wdt.CTRL.psel = (unsigned int) timeout;
    wdt.CTRL.en = 1;

    wdt.CTRL.key = 0x55;
    AVR32_WDT.ctrl = wdt.ctrl;

    wdt.CTRL.key = 0xaa;
    AVR32_WDT.ctrl = wdt.ctrl;
}

/* See wdt.h for details. */
void wdt_stop( void )
{
    avr32_wdt_t wdt;

    wdt.CTRL.psel = 31;
    wdt.CTRL.en = 0;

    wdt.CTRL.key = 0x55;
    AVR32_WDT.ctrl = wdt.ctrl;

    wdt.CTRL.key = 0xaa;
    AVR32_WDT.ctrl = wdt.ctrl;
}

/* See wdt.h for details. */
void wdt_heartbeat( void )
{
    AVR32_WDT.clr = 0xffffffff;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
