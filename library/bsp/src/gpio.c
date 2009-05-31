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

#include <avr32/io.h>

#include "gpio.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PIN_TO_PORT(pin)    (&AVR32_GPIO.port[(pin) >> 5])
#define PIN_TO_PIN(pin)     (1 << (0x1f & (pin)))

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
/* See gpio.h for details. */
bsp_status_t gpio_enable_module( const gpio_map_t *map, const size_t size )
{
    size_t i;

    if( (NULL == map) || (0 == size) ) {
        return BSP_ERROR_PARAMETER;
    }

    for( i = 0; i < size; i++ ) {
        gpio_enable_module_pin( map[i].pin, map[i].function );
    }

    return BSP_RETURN_OK;
}

/* See gpio.h for details. */
void gpio_enable_module_pin( const uint8_t pin, const uint8_t function )
{
    volatile avr32_gpio_port_t *port;
    uint32_t p;

    port = PIN_TO_PORT( pin );
    p = PIN_TO_PIN( pin );

    switch( function ) {
        case 0: /* A */
            port->pmr0c = p;
            port->pmr1c = p;

            port->gperc = p;
            break;

        case 1: /* B */
            port->pmr0s = p;
            port->pmr1c = p;

            port->gperc = p;
            break;

        case 2: /* C */
            port->pmr0c = p;
            port->pmr1s = p;

            port->gperc = p;
            break;

        case 3: /* D */
            port->pmr0s = p;
            port->pmr1s = p;

            port->gperc = p;
            break;

        default:
            port->oderc = p;
            port->gpers = p;
            break;
    }
}

/* See gpio.h for details. */
bsp_status_t gpio_set_options( const uint8_t pin,
                               const gpio_direction_t direction,
                               const gpio_pull_up_t pull_up,
                               const gpio_glitch_filter_t glitch_filter,
                               const gpio_interrupt_mode_t interrupt_mode,
                               const uint8_t initial_value )
{
    volatile avr32_gpio_port_t *port;
    uint32_t p;

    port = PIN_TO_PORT( pin );
    p = PIN_TO_PIN( pin );

    /* Check inputs first. */
    switch( direction ) {
        case GPIO_DIRECTION__INPUT:
        case GPIO_DIRECTION__BOTH:
        case GPIO_DIRECTION__OUTPUT:        break;
        default:                            return BSP_ERROR_PARAMETER;
    }

    switch( pull_up ) {
        case GPIO_PULL_UP__DISABLE:
        case GPIO_PULL_UP__ENABLE:          break;
        default:                            return BSP_ERROR_PARAMETER;
    }

    switch( glitch_filter ) {
        case GPIO_GLITCH_FILTER__DISABLE:
        case GPIO_GLITCH_FILTER__ENABLE:    break; 
        default:                            return BSP_ERROR_PARAMETER;
    }

    switch( interrupt_mode ) {
        case GPIO_INTERRUPT__NONE:
        case GPIO_INTERRUPT__CHANGE:
        case GPIO_INTERRUPT__RISING_EDGE:
        case GPIO_INTERRUPT__FALLING_EDGE:  break; 
        default:                            return BSP_ERROR_PARAMETER;
    }

    /* The input is all ok. */

    /* If we're input, do that part first so there are no glitches
     * due to ording of property setting */
    if( GPIO_DIRECTION__INPUT == direction ) {
        port->oderc = p;
    }

    if( GPIO_PULL_UP__DISABLE == pull_up ) {
        port->puerc = p;
    } else {
        port->puers = p;
    }

    if( (GPIO_GLITCH_FILTER__DISABLE == glitch_filter) &&
        (GPIO_INTERRUPT__NONE == interrupt_mode) )
    {
        port->gferc = p;
    } else {
        port->gfers = p;
    }

    switch( interrupt_mode ) {
        case GPIO_INTERRUPT__NONE:
            port->ierc = p;
            break;

        case GPIO_INTERRUPT__CHANGE:
            port->imr0c = p;
            port->imr1c = p;
            port->iers = p;
            break;

        case GPIO_INTERRUPT__RISING_EDGE:
            port->imr0s = p;
            port->imr1c = p;
            port->iers = p;
            break;

        case GPIO_INTERRUPT__FALLING_EDGE:
            port->imr0c = p;
            port->imr1s = p;
            port->iers = p;
            break;
    }

    /* Do this last so the output isn't "glitchy" due to odering
     * of the settings. */
    if( GPIO_DIRECTION__INPUT != direction ) {
        if( 0 == initial_value ) {
            port->ovrc = p;
        } else {
            port->ovrs = p;
        }

        port->oders = p;
    }

    return BSP_RETURN_OK;
}

/* See gpio.h for details. */
__inline__ void gpio_set_pin( const uint8_t pin )
{
    AVR32_GPIO.port[pin >> 5].ovrs = 1 << (0x1f & pin);
}

/* See gpio.h for details. */
__inline__ void gpio_clr_pin( const uint8_t pin )
{
    AVR32_GPIO.port[pin >> 5].ovrc = 1 << (0x1f & pin);
}

/* See gpio.h for details. */
__inline__ void gpio_tgl_pin( const uint8_t pin )
{
    AVR32_GPIO.port[pin >> 5].ovrt = 1 << (0x1f & pin);
}

/* See gpio.h for details. */
__inline__ uint32_t gpio_read_pin( const uint8_t pin )
{
    return (((AVR32_GPIO.port[pin >> 5].pvr) >> (0x1f & pin)) & 1);
}

/* See gpio.h for details. */
__inline__ uint32_t gpio_read_pin_output( const uint8_t pin )
{
    return (((AVR32_GPIO.port[pin >> 5].ovr) >> (0x1f & pin)) & 1);
}

/* See gpio.h for details. */
__inline__ void gpio_clr_interrupt_flag( const uint8_t pin )
{
    AVR32_GPIO.port[pin >> 5].ifrc = 1 << (0x1f & pin);
}

/* See gpio.h for details. */
void gpio_reset_pin( const uint8_t pin )
{
    volatile avr32_gpio_port_t *port;
    uint32_t p;

    port = PIN_TO_PORT( pin );
    p = PIN_TO_PIN( pin );

    /* Input Only */
    port->oderc = p;

    /* Disable any alternate functions. */
    port->gpers = p;

    /* Disable Open Drain */
    port->odmerc = p;

    /* Disable Pull Up Resister */
    port->puerc = p;

    /* Enable Glitch Filter */
    port->gfers = p;

    /* Disable ISR */
    port->ierc = p;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
