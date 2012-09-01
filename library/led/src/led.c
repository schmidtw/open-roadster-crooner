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

#include <freertos/os.h>

#include <bsp/pwm.h>
#include <bsp/boards/boards.h>

#include "led.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define LED_TASK_STACK_SIZE (50)
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
static queue_handle_t __idle;
static queue_handle_t __active;
static semaphore_handle_t __exit;

static led_msg_t __msgs[LED_MSG_MAX];

static const led_state_t __destroy = { .red = 0,
                                       .green = 0,
                                       .blue = 0,
                                       .duration = 0 };

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
    bool status;
    bsp_status_t bsp_status;

    static const gpio_map_t map[] = {
#if defined(LED_PWM_RED_PIN)
        { .pin = LED_PWM_RED_PIN,   .function = LED_PWM_RED_FUNCTION   }
#endif
#if defined(LED_PWM_GREEN_PIN)
        ,{ .pin = LED_PWM_GREEN_PIN, .function = LED_PWM_GREEN_FUNCTION }
#endif
#if defined(LED_PWM_BLUE_PIN)
        ,{ .pin = LED_PWM_BLUE_PIN,  .function = LED_PWM_BLUE_FUNCTION  }
#endif
    };

    __exit = os_semaphore_create_binary();
    __idle = os_queue_create( LED_MSG_MAX, sizeof(led_msg_t*) );
    __active = os_queue_create( LED_MSG_MAX, sizeof(led_msg_t*) );

    if( (NULL == __exit) || (NULL == __idle) || (NULL == __active) ) {
        goto failure;
    }

    os_semaphore_take( __exit, NO_WAIT );

    for( i = 0; i < LED_MSG_MAX; i++ ) {
        led_msg_t *msg = &__msgs[i];
        os_queue_send_to_back( __idle, &msg, NO_WAIT );
    }

    bsp_status = pwm_init( LED_CLOCK_FREQUENCY, 0, map,
                           sizeof(map)/sizeof(gpio_map_t) );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }

    /* Start in the off position, LEDs are active low. */
#if defined(LED_PWM_RED_PIN)
    bsp_status = pwm_channel_init( LED_PWM_RED_CHANNEL, PWM__HIGH, PWM__LEFT,
                                   LED_CLOCK_FREQUENCY, 255, 255 );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }
#endif
#if defined(LED_PWM_GREEN_PIN)
    bsp_status = pwm_channel_init( LED_PWM_GREEN_CHANNEL, PWM__HIGH, PWM__LEFT,
                                   LED_CLOCK_FREQUENCY, 255, 255 );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }
#endif
#if defined(LED_PWM_BLUE_PIN)
    bsp_status = pwm_channel_init( LED_PWM_BLUE_CHANNEL, PWM__HIGH, PWM__LEFT,
                                   LED_CLOCK_FREQUENCY, 255, 255 );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }
#endif

    bsp_status = pwm_start( (
#if defined(LED_PWM_RED_PIN)
                             (1 << LED_PWM_RED_CHANNEL) |
#endif
#if defined(LED_PWM_GREEN_PIN)
                             (1 << LED_PWM_GREEN_CHANNEL) |
#endif
#if defined(LED_PWM_BLUE_PIN)
                             (1 << LED_PWM_BLUE_CHANNEL) |
#endif
                             0) );
    if( BSP_RETURN_OK != bsp_status ) {
        goto failure;
    }

    status = os_task_create( __led_task, "LED", LED_TASK_STACK_SIZE,
                             NULL, priority, NULL );

    if( true != status ) {
        goto failure;
    }

    return LED_RETURN_OK;

failure:
    if( NULL != __exit ) {
        os_semaphore_delete( __exit );
    }

    if( NULL != __idle ) {
        os_queue_delete( __idle );
    }

    if( NULL != __active ) {
        os_queue_delete( __active );
    }

    return LED_RESOURCE_ERROR;
}

/* See led.h for details. */
void led_destroy( void )
{
    led_msg_t *in;

    led_set_state( (led_state_t*) &__destroy, 1, false, NULL );

    os_semaphore_take( __exit, WAIT_FOREVER );

    os_queue_delete( __idle );

    while( os_queue_receive(__active, &in, NO_WAIT) ) {
        if( NULL != in->free_fn ) {
            (*in->free_fn)( in->state );
        }
    }

    os_queue_delete( __active );

    os_semaphore_delete( __exit );
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

    os_queue_receive( __idle, &msg, WAIT_FOREVER );
    msg->state = states;
    msg->count = count;
    msg->repeat = repeat;
    msg->free_fn = free_fn;

    os_queue_send_to_back( __active, &msg, WAIT_FOREVER );

    return LED_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __led_task( void *params )
{
    bool done;
    led_msg_t *current;
    uint32_t delay;
    int step;

    step = 0;
    current = NULL;
    delay = WAIT_FOREVER;
    done = false;

    while( false == done ) {
        led_msg_t *in;

        if( true == os_queue_receive(__active, &in, delay) ) {
            /* We got a new command! */

            /* Exit if we are told to do so. */
            if( &__destroy == in->state ) {
                done = true;
                continue;
            }

            if( NULL != current ) {
                if( NULL != current->free_fn ) {
                    (*current->free_fn)( current->state );
                }
                current->state = NULL;
                os_queue_send_to_back( __idle, &current, WAIT_FOREVER );
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

            delay = current->state[step].duration;
            if( 0 == delay ) {
                delay = WAIT_FOREVER;
            }
            step++;
        } else {
            /* Shut off the LEDs and wait for instructions. */
            __set_colors( 0, 0, 0 );
            delay = WAIT_FOREVER;
        }
    }

    /* Turn off the leds before leaving. */
    __set_colors( 0, 0, 0 );

    os_semaphore_give( __exit );
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
#if defined(LED_PWM_RED_PIN)
    pwm_change_duty_cycle( LED_PWM_RED_CHANNEL,   (255 - red)   );
#endif
#if defined(LED_PWM_GREEN_PIN)
    pwm_change_duty_cycle( LED_PWM_GREEN_CHANNEL, (255 - green) );
#endif
#if defined(LED_PWM_BLUE_PIN)
    pwm_change_duty_cycle( LED_PWM_BLUE_CHANNEL,  (255 - blue)  );
#endif
}
