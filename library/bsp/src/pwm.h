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
#ifndef __PWM_H__
#define __PWM_H__

#include <stdint.h>

#include "gpio.h"
#include "pm.h"

typedef enum {
    PWM__HIGH,
    PWM__LOW
} pwm_polarity_t;

typedef enum {
    PWM__LEFT,
    PWM__CENTER
} pwm_alignment_t;

/**
 *  Used to initialize the entire PWM sub-system.
 *
 *  @param clock_a one of two prescaled clocks that can be used (Hz), based
 *                 on CPU clock
 *  @param clock_b one of two prescaled clocks that can be used (Hz), based
 *                 on CPU clock
 *  @param map the PWM channels to setup for output
 *  @param map_size the number of PWM channels to setup for output
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pwm_init( const uint32_t pba_clock,
                       const uint32_t pbb_clock,
                       const gpio_map_t *map,
                       const size_t map_size );

/**
 *  Used to initialize (but not start) a PWM channel.
 *
 *  @param channel the channel to initialize
 *  @param polarity the initial polarity to start from
 *  @param alignment the output signal alignment
 *  @param clock the clock frequency to use (Hz) (Must be a multiple of 2,
 *               of CPU clock, or clock_a or clock_b (See pwm_init)
 *  @param period the period in clock cycles, must be 0 < period < 0x00100000
 *  @param duty_cycle the duty cycle in clock cycles and must
 *                    be 0 < duty_cycle < period
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pwm_channel_init( const uint32_t channel,
                               const pwm_polarity_t polarity,
                               const pwm_alignment_t alignment,
                               const uint32_t clock,
                               const uint32_t period,
                               const uint32_t duty_cycle );

/**
 *  Used to start the bitmask of channels.
 *
 *  @param channels the channel bitmask to start
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pwm_start( const uint32_t channels );

/**
 *  Used to stop the bitmask of channels.
 *
 *  @param channels the channels bitmask to stop
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pwm_stop( const uint32_t channels );

/**
 *  Used to change the period of the PWM channel while running.
 *
 *  @note The new period must not be less than or equal to the current
 *        duty cycle.
 *
 *  @param channel the channel to change
 *  @param period the new period to use
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pwm_change_period( const uint32_t channel, const uint32_t period );

/**
 *  Used to change the duty cycle of the PWM channel while running.
 *
 *  @note The new duty cycle must be less than the current period.
 *
 *  @param channel the channel to change
 *  @param period the new period to use
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pwm_change_duty_cycle( const uint32_t channel,
                                    const uint32_t duty_cycle );
#endif
