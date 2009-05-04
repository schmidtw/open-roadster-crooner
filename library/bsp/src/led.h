/*
 * Open Roadster Crooner
 *
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

#ifndef __LED_H__
#define __LED_H__

typedef enum {
    led_none,
    led_green,
    led_red,
    led_blue,
    led_all
} led_t;

void led_init( void );

void led_on( const led_t led );
void led_off( const led_t led );
void led_toggle( const led_t led );
#endif
