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

#include "led.h"
#include <bsp/gpio.h>
#include <bsp/boards/boards.h>

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
void led_init( void )
{
#ifdef LED_GREEN
    gpio_set_options( LED_GREEN,
                      GPIO_DIRECTION__OUTPUT,
                      GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE,
                      GPIO_INTERRUPT__NONE, 1 );
#endif

#ifdef LED_RED
    gpio_set_options( LED_RED,
                      GPIO_DIRECTION__OUTPUT,
                      GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE,
                      GPIO_INTERRUPT__NONE, 1 );
#endif

#ifdef LED_BLUE
    gpio_set_options( LED_BLUE,
                      GPIO_DIRECTION__OUTPUT,
                      GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE,
                      GPIO_INTERRUPT__NONE, 1 );
#endif

    led_off( led_all );
}

void led_on( const led_t led )
{
#ifdef LED_GREEN
    if( (led_green == led) || (led_all == led) ) {
        gpio_clr_pin( LED_GREEN );
    }
#endif

#ifdef LED_RED
    if( (led_red == led) || (led_all == led) ) {
        gpio_clr_pin( LED_RED );
    }
#endif

#ifdef LED_BLUE
    if( (led_blue == led) || (led_all == led) ) {
        gpio_clr_pin( LED_BLUE );
    }
#endif
}

void led_off( const led_t led )
{
#ifdef LED_GREEN
    if( (led_green == led) || (led_all == led) ) {
        gpio_set_pin( LED_GREEN );
    }
#endif

#ifdef LED_RED
    if( (led_red == led) || (led_all == led) ) {
        gpio_set_pin( LED_RED );
    }
#endif

#ifdef LED_BLUE
    if( (led_blue == led) || (led_all == led) ) {
        gpio_set_pin( LED_BLUE );
    }
#endif
}

void led_toggle( const led_t led )
{
#ifdef LED_GREEN
    if( (led_green == led) || (led_all == led) ) {
        gpio_tgl_pin( LED_GREEN );
    }
#endif

#ifdef LED_RED
    if( (led_red == led) || (led_all == led) ) {
        gpio_tgl_pin( LED_RED );
    }
#endif

#ifdef LED_BLUE
    if( (led_blue == led) || (led_all == led) ) {
        gpio_tgl_pin( LED_BLUE );
    }
#endif
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
