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
#ifndef __LED_H__
#define __LED_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    LED_RETURN_OK,
    LED_PARAMETER_ERROR,
    LED_RESOURCE_ERROR
} led_status_t;

typedef struct {
    unsigned int red    :8;
    unsigned int green  :8;
    unsigned int blue   :8;
    unsigned int        :8;

    uint32_t duration;  /* in ms, 0 for forever */
} led_state_t;

/**
 *  Used to initialize the led subsystem.
 *
 *  @param priority the priority of the task
 *
 *  @return Status
 *      @retval LED_RETURN_OK       Success
 *      @retval LED_RESOURCE_ERROR  Resources aren't available
 */
led_status_t led_init( const uint32_t priority );

/**
 *  Used to set the colors and any transitions that take place.
 *
 *  @param states the states to output - memory is owned by the subsystem
 *  @param count the number of states
 *  @param repeat repeat the states until told to do something else
 *  @param free_fn the function to call to free this memory when we're
 *                 done with it (NULL to not free the memory)
 *
 *  @return Status
 *      @retval LED_RETURN_OK       Success
 *      @retval LED_PARAMETER_ERROR Invalid parameter
 */
led_status_t led_set_state( led_state_t *states,
                            const size_t count,
                            const bool repeat,
                            void (*free_fn)(void*) );
#endif
