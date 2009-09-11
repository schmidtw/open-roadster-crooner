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
#include <util/factor.h>

#include "gpio.h"
#include "pwm.h"
#include "pm.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PWM_CHANNEL_MASK    (0xffffffff >> (32 - AVR32_PWM_CHANNEL_LENGTH))

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static uint32_t __clock_a;
static uint32_t __clock_b;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void __calculate( const uint32_t clock, uint8_t *pre, uint8_t *div );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See pwm.h for details. */
bsp_status_t pwm_init( const uint32_t clock_a,
                       const uint32_t clock_b,
                       const gpio_map_t *map,
                       const size_t map_size )
{
    /* Disable all the channels */
    pwm_stop( PWM_CHANNEL_MASK );

    if( ((0 == clock_a) && (0 == clock_b)) ||
        (NULL == map) || (0 == map_size) )
    {
        return BSP_ERROR_PARAMETER;
    }

    if( 0 == clock_a ) {
        AVR32_PWM.MR.diva = 0;
    } else {
        uint8_t prea, diva;

        __calculate( clock_a, &prea, &diva );
        AVR32_PWM.MR.prea = prea;
        AVR32_PWM.MR.diva = diva;
    }

    if( 0 == clock_b ) {
        AVR32_PWM.MR.divb = 0;
    } else {
        uint8_t preb, divb;

        __calculate( clock_b, &preb, &divb );
        AVR32_PWM.MR.preb = preb;
        AVR32_PWM.MR.divb = divb;
    }

    gpio_enable_module( map, map_size );

    __clock_a = clock_a;
    __clock_b = clock_b;

    return BSP_RETURN_OK;
}

/* See pwm.h for details. */
bsp_status_t pwm_channel_init( const uint32_t channel,
                               const pwm_polarity_t polarity,
                               const pwm_alignment_t alignment,
                               const uint32_t clock,
                               const uint32_t period,
                               const uint32_t duty_cycle )
{
    /* Stop the channel if it is running. */
    AVR32_PWM.dis = (1 << channel);

    if( (AVR32_PWM_CHANNEL_LENGTH <= channel) ||
        (0 == clock) ||
        (0 != (0xFFF00000 & period)) || (0 == period) ||
        (period < duty_cycle) )
    {
        return BSP_ERROR_PARAMETER;
    }

    if( (__clock_a == clock) || (__clock_b == clock) ) {
        AVR32_PWM.channel[channel].CMR.cpre =
            (__clock_a == clock) ? AVR32_PWM_CLKA : AVR32_PWM_CLKB;
    } else {
        uint8_t pre;
        uint8_t div;

        __calculate( clock, &pre, &div );

        if( (0 == pre) || (0 != div) ) {
            return BSP_ERROR_PARAMETER;
        }

        AVR32_PWM.channel[channel].CMR.cpre = pre;
    }

    AVR32_PWM.channel[channel].CMR.cpol = (PWM__HIGH == polarity) ? 1 : 0;
    AVR32_PWM.channel[channel].CMR.calg = (PWM__CENTER == alignment) ? 1 : 0;
    AVR32_PWM.channel[channel].cdty = duty_cycle;
    AVR32_PWM.channel[channel].cprd = period;

    return BSP_RETURN_OK;
}

/* See pwm.h for details. */
bsp_status_t pwm_start( const uint32_t channels )
{
    if( 0 != (channels & (~(PWM_CHANNEL_MASK))) ) {
        return BSP_ERROR_PARAMETER;
    }

    AVR32_PWM.ena = channels;

    return BSP_RETURN_OK;
}

/* See pwm.h for details. */
bsp_status_t pwm_stop( const uint32_t channels )
{
    if( 0 != (channels & (~(PWM_CHANNEL_MASK))) ) {
        return BSP_ERROR_PARAMETER;
    }

    AVR32_PWM.dis = channels;

    return BSP_RETURN_OK;
}

/* See pwm.h for details. */
bsp_status_t pwm_change_period( const uint32_t channel, const uint32_t period )
{
    if( AVR32_PWM_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    if( period < AVR32_PWM.channel[channel].cdty ) {
        return BSP_ERROR_PARAMETER;
    }

    AVR32_PWM.channel[channel].CMR.cpd = 1;
    AVR32_PWM.channel[channel].cupd = period;

    return BSP_RETURN_OK;
}

/* See pwm.h for details. */
bsp_status_t pwm_change_duty_cycle( const uint32_t channel,
                                    const uint32_t duty_cycle )
{
    if( AVR32_PWM_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    if( AVR32_PWM.channel[channel].cprd < duty_cycle ) {
        return BSP_ERROR_PARAMETER;
    }

    AVR32_PWM.channel[channel].CMR.cpd = 0;
    AVR32_PWM.channel[channel].cupd = duty_cycle;

    return BSP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
void __calculate( const uint32_t clock, uint8_t *pre, uint8_t *div )
{
    uint32_t cpu_clock;
    uint32_t scale;

    cpu_clock = pm_get_frequency( PM__CPU );
    scale = cpu_clock / clock;

    *pre = 0;
    *div = 0;

    if( (scale * clock) == cpu_clock ) {
        bool done;
        uint32_t power;
        uint32_t remainder;

        factor_out_two( scale, &power, &remainder );

        done = false;
        while( false == done ) {
            switch( power ) {
                case 0:
#ifdef AVR32_PWM_MCK_DIV_2
                case 1:
#endif
#ifdef AVR32_PWM_MCK_DIV_4
                case 2:
#endif
#ifdef AVR32_PWM_MCK_DIV_8
                case 3:
#endif
#ifdef AVR32_PWM_MCK_DIV_16
                case 4:
#endif
#ifdef AVR32_PWM_MCK_DIV_32
                case 5:
#endif
#ifdef AVR32_PWM_MCK_DIV_64
                case 6:
#endif
#ifdef AVR32_PWM_MCK_DIV_128
                case 7:
#endif
#ifdef AVR32_PWM_MCK_DIV_256
                case 8:
#endif
#ifdef AVR32_PWM_MCK_DIV_512
                case 9:
#endif
#ifdef AVR32_PWM_MCK_DIV_1024
                case 10:
#endif
                    done = true;
                    break;

                default:
                    /* Try making the power less and the remainder larger. */
                    power--;
                    remainder <<= 1;
                    break;
            }
        }

        if( remainder < UINT8_MAX ) {
            *pre = power;
            *div = remainder;
        }
    }
}
