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

#include <stdbool.h>
#include <stdint.h>

#include <avr32/io.h>

#include "led.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PIN_TO_PORT(pin)    (&AVR32_GPIO.port[(pin) >> 5])
#define PIN_TO_PIN(pin)     (1 << (0x1f & (pin)))

#define LED_GPIO_BLUE_PIN       AVR32_PIN_PB19
#define LED_GPIO_GREEN_PIN      AVR32_PIN_PB20
#define LED_GPIO_RED_PIN        AVR32_PIN_PB21

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
static void __led_init_one( const uint8_t pin );
static void __led_set( const uint8_t pin, const bool on );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void led_init( void )
{
    __led_init_one( LED_GPIO_RED_PIN );
    __led_init_one( LED_GPIO_GREEN_PIN );
    __led_init_one( LED_GPIO_BLUE_PIN );
}

void led_off( void )
{
    __led_set( LED_GPIO_RED_PIN,   false );
    __led_set( LED_GPIO_GREEN_PIN, false );
    __led_set( LED_GPIO_BLUE_PIN,  false );
}

void led_active( void )
{
    led_off();

    /* Yellow */
    __led_set( LED_GPIO_RED_PIN, true );
    __led_set( LED_GPIO_GREEN_PIN, true );
}

void led_normal( void )
{
    led_off();

    /* Blue */
    __led_set( LED_GPIO_BLUE_PIN, true );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __led_init_one( const uint8_t pin )
{
    volatile avr32_gpio_port_t *port;
    uint32_t p;

    port = PIN_TO_PORT( pin );
    p = PIN_TO_PIN( pin );

    port->puers = p;    /* Enable Pull Up */
    port->ovrs  = p;    /* Set to LED off */
    port->oders = p;    /* Out */
}

static void __led_set( const uint8_t pin, const bool on )
{
    volatile avr32_gpio_port_t *port;
    uint32_t p;

    port = PIN_TO_PORT( pin );
    p = PIN_TO_PIN( pin );

    if( true == on ) {
        port->ovrc = p;    /* Set to LED on */
    } else {
        port->ovrs = p;    /* Set to LED off */
    }
}
