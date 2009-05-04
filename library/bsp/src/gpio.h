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
#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>
#include <stddef.h>

#include "bsp_errors.h"

typedef enum {
    GPIO_PULL_UP__DISABLE,
    GPIO_PULL_UP__ENABLE
} gpio_pull_up_t;

typedef enum {
    GPIO_GLITCH_FILTER__DISABLE,
    GPIO_GLITCH_FILTER__ENABLE
} gpio_glitch_filter_t;

typedef enum {
    GPIO_DIRECTION__INPUT,
    GPIO_DIRECTION__BOTH,
    GPIO_DIRECTION__OUTPUT
} gpio_direction_t;

typedef enum {
    GPIO_INTERRUPT__NONE,
    GPIO_INTERRUPT__CHANGE,
    GPIO_INTERRUPT__RISING_EDGE,
    GPIO_INTERRUPT__FALLING_EDGE
} gpio_interrupt_mode_t;

typedef struct {
    uint8_t pin;
    uint8_t function;
} gpio_map_t;

/**
 *  Used to set a number of pin:function mappings at once.
 *
 *  @param map the pointer to the pin:function mapping structure array
 *  @param size the count of array items
 *
 *  @retval BSP_RETURN_OK
 *  @retval BSP_ERROR_PARAMETER
 */
bsp_status_t gpio_enable_module( const gpio_map_t *map, const size_t size );

/**
 *  Used to set a pin to a particular function.
 *
 *  @param pin the pin to set the function for
 *  @param function [0-3] for the module [ABCD], any other value results
 *                  in the pin being defined as gpio, set up in an input
 *                  configuration.
 */
void gpio_enable_module_pin( const uint8_t pin, const uint8_t function );

/**
 *  Used to set the options for a pin.  No options are set on an error.
 *
 *  @param direction the direction of the pin [input|output|both]
 *  @param pull_up if the pin should have the pull-up enabled
 *  @param glitch_filter if the pin should have the glitch filter enabled
 *  @param interrupt_mode the interrupt mode used for the pin - if this
 *                        is set, the the glitch filter is automatically
 *                        enabled
 *  @param initial_value the initial value of the output of the pin -
 *                       only set if direction is [output|both]
 *
 *  @retval BSP_RETURN_OK
 *  @retval BSP_ERROR_PARAMETER
 */
bsp_status_t gpio_set_options( const uint8_t pin,
                               const gpio_direction_t direction,
                               const gpio_pull_up_t pull_up,
                               const gpio_glitch_filter_t glitch_filter,
                               const gpio_interrupt_mode_t interrupt_mode,
                               const uint8_t initial_value );

/**
 *  Used to set a pin to the high/on/'1'/Vcc output state if
 *  the pin is setup to support output, otherwise the state is
 *  set, but is not visible on the physical output of the pin.
 *
 *  @param pin the pin to set
 */
__inline__ void gpio_set_pin( const uint8_t pin );

/**
 *  Used to set a pin to the low/off/'0'/Gnd output state if
 *  the pin is setup to support output, otherwise the state is
 *  set, but is not visible on the physical output of the pin.
 *
 *  @param pin the pin to clear
 */
__inline__ void gpio_clr_pin( const uint8_t pin );

/**
 *  Used to toggle a pin from '0' to '1' or '1' to '0' depending
 *  on the current state of the pin.  If the pin is not set
 *  to output, the operation still takes place, but the output
 *  of the pin is not visible on the physical output of the pin.
 *
 *  @param pin the pin to toggle
 */
__inline__ void gpio_tgl_pin( const uint8_t pin );

/**
 *  Used to read the input value from a pin.
 *
 *  @param pin the pin to read
 *
 *  @retval 0 if the pin input is low/off/'0'/Gnd or not in GPIO mode
 *  @retval 1 if the pin input is high/on/'1'/Vcc
 */
__inline__ uint32_t gpio_read_pin( const uint8_t pin );

/**
 *  Used to read the output value from a pin.
 *
 *  @param pin the pin to read
 *
 *  @retval 0 if the pin output is low/off/'0'/Gnd or not in GPIO mode
 *  @retval 1 if the pin output is high/on/'1'/Vcc
 */
__inline__ uint32_t gpio_read_pin_output( const uint8_t pin );

/**
 *  Used to clear the interrupt flag of a gpio pin.
 *
 *  @param pin the pin who's interrupt is to be cleared
 */
__inline__ void gpio_clr_interrupt_flag( const uint8_t pin );

/**
 *  Used to return a pin to it's initial reset state of:
 *  input only, open drain disabled, pullup disabled, glitch filter
 *  enabled, no interrupts enabled.
 *
 *  @param pin the pin to reset
 */
void gpio_reset_pin( const uint8_t pin );
#endif
