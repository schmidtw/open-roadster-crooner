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

#ifndef __WDT_H__
#define __WDT_H__

typedef enum {
    WDT__17us = 0,
    WDT__35us,
    WDT__70us,
    WDT__139us,
    WDT__278us,
    WDT__557us,
    WDT__1ms,
    WDT__2ms,
    WDT__4ms,
    WDT__8ms,
    WDT__17ms,
    WDT__35ms,
    WDT__71ms,
    WDT__142ms,
    WDT__284ms,
    WDT__569ms,
    WDT__1s,
    WDT__2s,
    WDT__4s,
    WDT__9s,
    WDT__18s,
    WDT__36s,
    WDT__72s,
    WDT__145s,
    WDT__291s,
    WDT__583s,
    WDT__1167s,
    WDT__2334s,
    WDT__4668s,
    WDT__9336s,
    WDT__18673s,
    WDT__37347s
} wdt_timeout_t;

/**
 *  Used to start the watchdog timeout system.
 *
 *  @param timeout the time before the watchdog generates a reset
 */
void wdt_start( const wdt_timeout_t timeout );

/**
 *  Used to stop the watchdog timeout system.
 */
void wdt_stop( void );

/**
 *  Used to keep the watchdog from rebooting the system.
 */
void wdt_heartbeat( void );
#endif
