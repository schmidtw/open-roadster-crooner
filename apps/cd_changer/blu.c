/*
 * <PROJECT>
 *
 * <AUTHOR>
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
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <bsp/gpio.h>
#include <string.h>

#include <bsp/led.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "blu.h"
#include "ibus.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define BLU_DEBUG   1

#define BLU_TASK_STACK_SIZE             100
#define BLU_TASK_PRIORITY               (tskIDLE_PRIORITY+1)
#define BLU_MAX_MESSAGES                10
#define BLU_ANNOUNCE_RETRY_TIMEOUT      (1*1000)
#define BLU_ANNOUNCE_RETRY_COUNT        30
#define BLU_DISC_STATUS_MAX_MESSAGES    3

#define BLU_MUTEX_LOCK()                xSemaphoreTake( __blu_mutex, portMAX_DELAY )
#define BLU_MUTEX_UNLOCK()              xSemaphoreGive( __blu_mutex )

#define _D1(...)
#define _D2(...)

#if (0 < BLU_DEBUG)
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (1 < BLU_DEBUG)
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    BLU_LIFECYCLE_STATE__NOT_CONNECTED,
    BLU_LIFECYCLE_STATE__CONNECTED
} lifecycle_states_t;

typedef enum {
    BLU_DISC_STATE__NO_MAGAZINE,
    BLU_DISC_STATE__NO_DISCS,
    BLU_DISC_STATE__ENUMERATING,
    BLU_DISC_STATE__DISCS,
} disc_states_t;

typedef enum {
    BLU_PLAY_STATE__STOPPED,
    BLU_PLAY_STATE__PLAYING,
    BLU_PLAY_STATE__PAUSED
} play_states_t;

typedef struct {
    bool in_use;
    disc_status_message_t disc_status;
} disc_status_message_wrapper_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static disc_status_message_wrapper_t __disc_status[BLU_DISC_STATUS_MAX_MESSAGES];
static xSemaphoreHandle __blu_mutex;
static xQueueHandle __blu_queue;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void __blu_task( void *params );
void __check_for_disc( const uint8_t complete_mask, const uint8_t current_disc,
                       const uint32_t delay, const ibus_audio_state_t audio_state );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void blu_init( void )
{
    led_init();

    __blu_mutex = xSemaphoreCreateMutex();
    __blu_queue = xQueueCreate( BLU_MAX_MESSAGES, sizeof(void*) );

    xTaskCreate( __blu_task, ( signed portCHAR *) "BLU ",
                 BLU_TASK_STACK_SIZE, NULL, BLU_TASK_PRIORITY, NULL );
}

disc_status_message_t *disc_status_message_alloc( void )
{
    size_t i;

    while( 1 ) {
        BLU_MUTEX_LOCK();

        for( i = 0; i < BLU_DISC_STATUS_MAX_MESSAGES; i++ ) {
            if( false == __disc_status[i].in_use ) {
                __disc_status[i].in_use = true;
                BLU_MUTEX_UNLOCK();

                __disc_status[i].disc_status.type = MT_DISC__STATUS;
                return &__disc_status[i].disc_status;
            }
        }

        BLU_MUTEX_UNLOCK();

        /* Try until we succeed. */
        vTaskDelay( 100 );
    }
}

void blu_message_free( void *msg )
{
    size_t i;

    BLU_MUTEX_LOCK();

    for( i = 0; i < BLU_DISC_STATUS_MAX_MESSAGES; i++ ) {
        if( msg == &__disc_status[i].disc_status ) {
            __disc_status[i].in_use = false;
            goto done;
        }
    }

done:

    BLU_MUTEX_UNLOCK();
}

void blu_message_post( void *msg )
{
    while( pdTRUE != xQueueSendToBack(__blu_queue, &msg, 0) ) {
        vTaskDelay( 100 );
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void __blu_task( void *params )
{
    ibus_tx_message_t *send;
    lifecycle_states_t lifecycle_state;
    disc_states_t disc_state;
    play_states_t play_state;
    uint8_t discs;
    uint8_t current_disc;
    uint8_t current_track;
    ibus_audio_state_t audio_state;
    int count;
    portTickType wait_time;

    wait_time = BLU_ANNOUNCE_RETRY_TIMEOUT;
    _D1( "lifecycle_state: NOT CONNECTED\n" );
    lifecycle_state = BLU_LIFECYCLE_STATE__NOT_CONNECTED;
    disc_state = BLU_DISC_STATE__NO_MAGAZINE;
    play_state = BLU_PLAY_STATE__STOPPED;

    discs = 0;
    current_disc = 0;
    current_track = 0;
    audio_state = IBUS_AUDIO_STATE__STOPPED;

    ibus_register( __blu_queue );

    led_on( led_red );

    count = BLU_ANNOUNCE_RETRY_COUNT;
#if 0
    /* Announce our existance. */
    while( BLU_LIFECYCLE_STATE__NOT_CONNECTED == lifecycle_state ) {
        ibus_tx_message_t *send;
        ibus_rx_message_t *receive;

        send = ibus_message_alloc();
        send->type = MT_IBUS__TX_MESSAGE;
        send->cmd = IBUS_TX_CMD__ANNOUNCE;
        ibus_message_post( send );
        if( pdTRUE == xQueueReceive(__blu_queue, &receive, BLU_ANNOUNCE_RETRY_TIMEOUT) ) {
            if( NULL != receive ) {
                /* We're connected until we are powered down. */
                lifecycle_state = BLU_LIFECYCLE_STATE__CONNECTED;

                ibus_message_free( receive );
            }
        } else {
            count++;
            if( 10 == count ) {
                lifecycle_state = BLU_LIFECYCLE_STATE__CONNECTED;

            }
        }
        led_toggle( led_red );
    }
    led_off( led_red );

    vTaskDelay( 1000 );
    led_on( led_green );
    vTaskDelay( 1000 );
    led_off( led_green );
    vTaskDelay( 1000 );
    /* Send our initial state. */
    send = ibus_message_alloc();
    send->type = MT_IBUS__TX_MESSAGE;
    send->cmd = IBUS_TX_CMD__STOPPED;
    send->d.status.audio_state = audio_state;
    send->d.status.current_disc = current_disc;
    send->d.status.current_track = current_track;
    send->d.status.disc_status = discs;
    ibus_message_post( send );
#endif

    while( 1 ) {
        ibus_rx_message_t *receive;

        _D2( "blu --->\n" );
        if( pdTRUE == xQueueReceive(__blu_queue, &receive, wait_time) ) {
            _D2( "blu <---\n" );

            if( NULL != receive ) {
                _D2( "got message\n" );
                if( MT_IBUS__RX_MESSAGE == receive->type ) {
                    if( BLU_LIFECYCLE_STATE__NOT_CONNECTED == lifecycle_state ) {
                        /* We're attached - make the message wait time 'forever' */
                        _D1( "lifecycle_state: CONNECTED\n" );
                        lifecycle_state = BLU_LIFECYCLE_STATE__CONNECTED;
                        wait_time = portMAX_DELAY;
                        led_off( led_red );
                        vTaskDelay( 200 );
                        led_on( led_red );
                        vTaskDelay( 200 );
                        led_off( led_red );
                        vTaskDelay( 200 );
                        led_on( led_red );
                        vTaskDelay( 200 );
                        led_off( led_red );
                    }

                    send = ibus_message_alloc();
                    send->type = MT_IBUS__TX_MESSAGE;
                    switch( receive->cmd ) {
                        case IBUS_RX_CMD__STATUS:
                            _D1( "Status Request\n" );
                            break;
                        case IBUS_RX_CMD__STOP:
                            _D1( "Command Stop\n" );
                            audio_state = IBUS_AUDIO_STATE__STOPPED;
                            send->cmd = IBUS_TX_CMD__STOPPED;
                            break;
                        case IBUS_RX_CMD__PAUSE:
                            _D1( "Command Pause\n" );
                            audio_state = IBUS_AUDIO_STATE__PAUSED;
                            send->cmd = IBUS_TX_CMD__PAUSED;
                            break;
                        case IBUS_RX_CMD__PLAY:
                            _D1( "Command Play\n" );
                            audio_state = IBUS_AUDIO_STATE__PLAYING;
                            send->cmd = IBUS_TX_CMD__PLAYING;
                            break;
                        case IBUS_RX_CMD__FAST_PLAY:
                            _D1( "Command Fast Play\n" );
                            audio_state = IBUS_AUDIO_STATE__PLAYING;
                            send->cmd = IBUS_TX_CMD__FAST_PLAYING;
                            break;
                        case IBUS_RX_CMD__SEEK:
                            _D1( "Command Seek\n" );
                            break;
                        case IBUS_RX_CMD__CHANGE_DISC:
                            _D1( "Command Change Disc\n" );
                            break;
                        case IBUS_RX_CMD__SCAN_DISC:
                            _D1( "Command Scan Disc\n" );
                            break;
                        case IBUS_RX_CMD__RANDOMIZE:
                            _D1( "Command Randomize Disc\n" );
                            break;
                        case IBUS_RX_CMD__POLL:
                            _D1( "Poll\n" );
                            send->cmd = IBUS_TX_CMD__POLL_RESPONSE;
                            break;
                    }

                    if( IBUS_RX_CMD__POLL != receive->cmd ) {
                        send->d.status.audio_state = audio_state;
                        send->d.status.current_disc = current_disc;
                        send->d.status.current_track = current_track;
                        send->d.status.disc_status = discs;
                    }

                    ibus_message_post( send );

                    ibus_message_free( receive );
                } else if( MT_DISC__STATUS == receive->type ) {
                    disc_status_message_t *disc_status;

                    disc_status = (disc_status_message_t *) receive;

                    if( discs != disc_status->disc_status ) {
                        discs = disc_status->disc_status;
                        if( 0 == discs ) {
                            /* Go to NO_MAGAZINE state */
                            _D1( "NO MAGAZINE state\n" );
                            disc_state = BLU_DISC_STATE__NO_MAGAZINE;
                            audio_state = IBUS_AUDIO_STATE__STOPPED;
                            current_disc = 0;
                            current_track = 0;

                            send = ibus_message_alloc();
                            send->type = MT_IBUS__TX_MESSAGE;
                            send->cmd = IBUS_TX_CMD__STOPPED;
                            send->d.status.audio_state = audio_state;
                            send->d.status.current_disc = current_disc;
                            send->d.status.current_track = current_track;
                            send->d.status.disc_status = discs;
                            ibus_message_post( send );
                        } else if( 0 == (BLU_DISC_STATUS__DISC_ANY & discs) ) {
                            /* Magazine, but no discs */
                            _D1( "NO DISC state\n" );
                            disc_state = BLU_DISC_STATE__NO_DISCS;
                            audio_state = IBUS_AUDIO_STATE__STOPPED;
                            current_disc = 0;
                            current_track = 0;

                            send = ibus_message_alloc();
                            send->type = MT_IBUS__TX_MESSAGE;
                            send->cmd = IBUS_TX_CMD__STOPPED;
                            send->d.status.audio_state = audio_state;
                            send->d.status.current_disc = current_disc;
                            send->d.status.current_track = current_track;
                            send->d.status.disc_status = discs;
                            ibus_message_post( send );
                        } else {
                            /* Enumerate */

                            int i;

                            _D1( "Walk discs state\n" );

                            for( i = 1; i <= 6; i++ ) {
                                __check_for_disc( discs, i, 100, audio_state );
                                vTaskDelay( 100 );
                            }

                            current_disc = 2;
                            current_track = 1;

                            send = ibus_message_alloc();
                            send->type = MT_IBUS__TX_MESSAGE;
                            send->cmd = IBUS_TX_CMD__STOPPED;
                            send->d.status.audio_state = audio_state;
                            send->d.status.current_disc = current_disc;
                            send->d.status.current_track = current_track;
                            send->d.status.disc_status = discs;
                            ibus_message_post( send );
                        }
                    }

                    blu_message_free( receive );
                }
                receive = NULL;
            }
        } else {
            _D2( "blu <---\n" );
            if( BLU_LIFECYCLE_STATE__NOT_CONNECTED == lifecycle_state ) {
                count++;
                if( BLU_ANNOUNCE_RETRY_COUNT <= count ) {
                    ibus_tx_message_t *send;

                    /* Send a new announcement */
                    send = ibus_message_alloc();
                    send->type = MT_IBUS__TX_MESSAGE;
                    send->cmd = IBUS_TX_CMD__ANNOUNCE;
                    ibus_message_post( send );

                    count = 0;
                }

                led_toggle( led_red );
            }
        }
    }
}

void __check_for_disc( const uint8_t complete_mask, const uint8_t current_disc,
                       const uint32_t delay, const ibus_audio_state_t audio_state )
{
    uint8_t before_mask, current_mask, current;
    ibus_tx_message_t *send;

    before_mask = 0;
    current_mask = 0;
    current = 0;

    switch( current_disc ) {
        case 6: current = BLU_DISC_STATUS__DISC_6 & complete_mask;  break;
        case 5: current = BLU_DISC_STATUS__DISC_5 & complete_mask;  break;
        case 4: current = BLU_DISC_STATUS__DISC_4 & complete_mask;  break;
        case 3: current = BLU_DISC_STATUS__DISC_3 & complete_mask;  break;
        case 2: current = BLU_DISC_STATUS__DISC_2 & complete_mask;  break;
        case 1: current = BLU_DISC_STATUS__DISC_1 & complete_mask;  break;
    }

    /* This is funny, but intentional - the radio wants the mask from before. */
    switch( current_disc ) {
        case 6: before_mask |= BLU_DISC_STATUS__DISC_5;
        case 5: before_mask |= BLU_DISC_STATUS__DISC_4;
        case 4: before_mask |= BLU_DISC_STATUS__DISC_3;
        case 3: before_mask |= BLU_DISC_STATUS__DISC_2;
        case 2: before_mask |= BLU_DISC_STATUS__DISC_1;
    }

    send = ibus_message_alloc();
    switch( current_disc ) {
        case 6: current_mask |= BLU_DISC_STATUS__DISC_6;
        case 5: current_mask |= BLU_DISC_STATUS__DISC_5;
        case 4: current_mask |= BLU_DISC_STATUS__DISC_4;
        case 3: current_mask |= BLU_DISC_STATUS__DISC_3;
        case 2: current_mask |= BLU_DISC_STATUS__DISC_2;
        case 1: current_mask |= BLU_DISC_STATUS__DISC_1;
    }

    send = ibus_message_alloc();
    send->type = MT_IBUS__TX_MESSAGE;
    send->cmd = IBUS_TX_CMD__CHECKING_FOR_DISC;
    send->d.check_for_disc.audio_state = audio_state;
    send->d.check_for_disc.mask = before_mask & complete_mask;
    send->d.check_for_disc.last_failed = false;
    send->d.check_for_disc.disc = current_disc;
    ibus_message_post( send );

        vTaskDelay( delay );

    send = ibus_message_alloc();
    send->type = MT_IBUS__TX_MESSAGE;
    send->cmd = IBUS_TX_CMD__CHECKING_FOR_DISC;
    send->d.check_for_disc.audio_state = audio_state;
    send->d.check_for_disc.mask = current_mask & complete_mask;
    send->d.check_for_disc.last_failed = (0 == current);
    send->d.check_for_disc.disc = current_disc;
    ibus_message_post( send );
}
