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
#include <string.h>

#include <freertos/task.h>
#include <freertos/queue.h>

#include <bsp/pwm.h>
#include <bsp/boards/boards.h>

#include "led.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define LED_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define LED_MSG_MAX         2
#define LED_CLOCK_FREQUENCY 103125

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    led_state_t *state;
    size_t count;
    bool repeat;
    void (*free_fn)(void*);
} led_msg_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xQueueHandle __idle;
static xQueueHandle __active;

static led_msg_t __msgs[LED_MSG_MAX];

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __led_task( void *params );
static void __set_colors( const uint8_t red,
                          const uint8_t green,
                          const uint8_t blue );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See led.h for details. */
led_status_t led_init( const uint32_t priority )
{
    int i;
    portBASE_TYPE status;
    bsp_status_t bsp_status;

    static const gpio_map_t map[] = {
        { .pin = LED_PWM_RED_PIN,   .function = LED_PWM_RED_FUNCTION   },
        { .pin = LED_PWM_GREEN_PIN, .function = LED_PWM_GREEN_FUNCTION },
        { .pin = LED_PWM_BLUE_PIN,  .function = LED_PWM_BLUE_FUNCTION  }
    };

    __idle = xQueueCreate( LED_MSG_MAX, sizeof(led_msg_t*) );
    __active = xQueueCreate( LED_MSG_MAX, sizeof(led_msg_t*) );

    if( (NULL == __idle) || (NULL == __active) ) {
        goto failure;
    }

    for( i = 0; i < LED_MSG_MAX; i++ ) {
        led_msg_t *msg = &__msgs[i];
        xQueueSendToBack( __idle, &msg, 0 );
    }

    bsp_status = pwm_init( LED_CLOCK_FREQUENCY, 0, map,
                           sizeof(map)/sizeof(gpio_map_t) );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }

    /* Start in the off position, LEDs are active low. */
    bsp_status = pwm_channel_init( LED_PWM_RED_CHANNEL, PWM__HIGH, PWM__LEFT,
                                   LED_CLOCK_FREQUENCY, 255, 255 );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }
    bsp_status = pwm_channel_init( LED_PWM_GREEN_CHANNEL, PWM__HIGH, PWM__LEFT,
                                   LED_CLOCK_FREQUENCY, 255, 255 );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }
    bsp_status = pwm_channel_init( LED_PWM_BLUE_CHANNEL, PWM__HIGH, PWM__LEFT,
                                   LED_CLOCK_FREQUENCY, 255, 255 );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }

    bsp_status = pwm_start( ((1 << LED_PWM_RED_CHANNEL) |
                             (1 << LED_PWM_GREEN_CHANNEL) |
                             (1 << LED_PWM_BLUE_CHANNEL)) );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }

    status = xTaskCreate( __led_task, ( signed portCHAR *) "LED",
                          LED_TASK_STACK_SIZE, NULL, priority, NULL );

    if( pdPASS != status ) {
        goto failure;
    }

    return LED_RETURN_OK;

failure:
    return LED_RESOURCE_ERROR;
}

/* See led.h for details. */
led_status_t led_set_state( led_state_t *states,
                            const size_t count,
                            const bool repeat,
                            void (*free_fn)(void*) )
{
    led_msg_t *msg;

    if( (NULL == states) || (0 == count) ) {
        return LED_PARAMETER_ERROR;
    }

    xQueueReceive( __idle, &msg, portMAX_DELAY );
    msg->state = states;
    msg->count = count;
    msg->repeat = repeat;
    msg->free_fn = free_fn;

    xQueueSendToBack( __active, &msg, portMAX_DELAY );

    return LED_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __led_task( void *params )
{
    led_msg_t *current;
    portTickType delay;
    int step;

    step = 0;
    current = NULL;
    delay = portMAX_DELAY;

    while( 1 ) {
        led_msg_t *in;

        if( pdTRUE == xQueueReceive(__active, &in, delay) ) {
            /* We got a new command! */
            if( NULL != current ) {
                if( NULL != current->free_fn ) {
                    (*current->free_fn)( current->state );
                }
                current->state = NULL;
                xQueueSendToBack( __idle, &current, portMAX_DELAY );
            }
            current = in;
            in = NULL;
            step = 0;
        }

        if( (current->count <= step) && (true == current->repeat) ) {
            step = 0;
        }

        if( step < current->count ) {
            /* Time to execute the next state */
            __set_colors( current->state[step].red,
                          current->state[step].green,
                          current->state[step].blue );

            delay = TASK_DELAY_MS( current->state[step].duration );
            if( 0 == delay ) {
                delay = portMAX_DELAY;
            }
            step++;
        } else {
            /* Shut off the LEDs and wait for instructions. */
            __set_colors( 0, 0, 0 );
            delay = portMAX_DELAY;
        }
    }
}

/**
 *  Set the output color.
 *
 *              (R,   G,   B)
 *  Black/Off = (0,   0,   0)
 *  Red       = (255, 0,   0)
 *  Blue      = (0,   0,   255)
 *  White     = (255, 255, 255)
 */
static void __set_colors( const uint8_t red,
                          const uint8_t green,
                          const uint8_t blue )
{
    /* Invert because the LED is active low. */
    pwm_change_duty_cycle( LED_PWM_RED_CHANNEL,   (255 - red)   );
    pwm_change_duty_cycle( LED_PWM_GREEN_CHANNEL, (255 - green) );
    pwm_change_duty_cycle( LED_PWM_BLUE_CHANNEL,  (255 - blue)  );
}
