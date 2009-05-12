/*
 * (c) 2008-2009 Open Roadster
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <bsp/led.h>
#include <bsp/gpio.h>
#include <bsp/usart.h>
#include <bsp/pdca.h>
#include <bsp/intc.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <ibus-physical/ibus-physical.h>

#include "messages.h"

#include "ibus.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DEBUG_IBUS  0

#define IBUS_RX_TASK_STACK_SIZE 1000
#define IBUS_TX_TASK_STACK_SIZE 1000
#define IBUS_RX_TASK_PRIORITY   (tskIDLE_PRIORITY+1)
#define IBUS_TX_TASK_PRIORITY   (tskIDLE_PRIORITY+1)
#define IBUS_TX_PDCA_CHANNEL    5

#define IBUS_MAX_TX_LENGTH      20
#define IBUS_MESSAGE_MAX_SIZE   250
#define IBUS_MESSAGE_TIMEOUT    10
#define IBUS_MAX_TX_MESSAGES    25
#define IBUS_MAX_RX_MESSAGES    20
#define IBUS_MAX_MESSAGES       (IBUS_MAX_TX_MESSAGES + IBUS_MAX_RX_MESSAGES)

#define IBUS_MUTEX_LOCK()       xSemaphoreTake( __ibus_mutex, portMAX_DELAY )
#define IBUS_MUTEX_UNLOCK()     xSemaphoreGive( __ibus_mutex )

#define ENABLE_CTS_ISR()        { IBUS_USART->IER.ctsic = 1; }
#define DISABLE_CTS_ISR()       { IBUS_USART->IDR.ctsic = 1; }

#define IBUS_DEVICE_RADIO       0x68
#define IBUS_DEVICE_CHANGER     0x18
#define IBUS_BROADCAST_HIGH     0xff
#define IBUS_BROADCAST_LOW      0x00

#define _D1(...)
#define _D2(...)

#if (0 < DEBUG_IBUS)
#undef  _D1
#define _D1(...)    printf( __VA_ARGS__ )
#endif
#if (1 < DEBUG_IBUS)
#undef  _D2
#define _D2(...)    printf( __VA_ARGS__ )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    bool in_use;
    ibus_rx_message_t rx;
} ibus_rx_message_wrapper_t;

typedef struct {
    bool in_use;
    ibus_tx_message_t tx;
} ibus_tx_message_wrapper_t;

typedef enum {
    IBUS_TX_STATUS__IDLE,
    IBUS_TX_STATUS__WAITING_FOR_CTS,
    IBUS_TX_STATUS__SENDING,
    IBUS_TX_STATUS__INTERRUPTED,
    IBUS_TX_STATUS__COMPLETE
} ibus_tx_status_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static ibus_tx_message_wrapper_t __tx[IBUS_MAX_TX_MESSAGES];
static ibus_rx_message_wrapper_t __rx[IBUS_MAX_RX_MESSAGES];
static xSemaphoreHandle __ibus_mutex;
static xQueueHandle __ibus_listener;
static xQueueHandle __ibus_queue;

volatile ibus_tx_status_t __tx_status;
volatile bool __tx_complete;
volatile bool __tx_interrupted;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __ibus_task_rx( void *data );
static void __ibus_task_tx( void *data );
static void __process_message( ibus_io_msg_t *msg );
ibus_rx_message_t *__get_rx_message( void );
void __ibus_send_msg( const uint8_t src, const uint8_t dst,
                      const uint8_t *msg, const uint8_t length );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void ibus_init( void )
{
    ibus_physical_init();

    __ibus_mutex = xSemaphoreCreateMutex();

    __ibus_queue = xQueueCreate( IBUS_MAX_MESSAGES, sizeof(void*) );


    xTaskCreate( __ibus_task_rx, ( signed portCHAR *) "iBrx",
                 IBUS_RX_TASK_STACK_SIZE, NULL, IBUS_RX_TASK_PRIORITY, NULL );

    xTaskCreate( __ibus_task_tx, ( signed portCHAR *) "iBtx",
                 IBUS_TX_TASK_STACK_SIZE, NULL, IBUS_TX_TASK_PRIORITY, NULL );
}

int32_t ibus_register( xQueueHandle listener )
{
    __ibus_listener = listener;

    return 0;
}

int32_t ibus_deregister( xQueueHandle incoming )
{
    __ibus_listener = NULL;
    return 0;
}

ibus_tx_message_t *ibus_message_alloc( void )
{
    size_t i;

    while( 1 ) {
        IBUS_MUTEX_LOCK();

        for( i = 0; i < IBUS_MAX_TX_MESSAGES; i++ ) {
            if( false == __tx[i].in_use ) {
                __tx[i].in_use = true;
                IBUS_MUTEX_UNLOCK();

                __tx[i].tx.type = MT_IBUS__TX_MESSAGE;
                return &__tx[i].tx;
            }
        }

        IBUS_MUTEX_UNLOCK();

        /* Retry until we succeed. */
        vTaskDelay( 100 );
    }
}

void ibus_message_free( void *msg )
{
    size_t i;

    IBUS_MUTEX_LOCK();

    for( i = 0; i < IBUS_MAX_TX_MESSAGES; i++ ) {
        if( msg == &__tx[i].tx ) {
            __tx[i].in_use = false;
            goto done;
        }
    }
    
    for( i = 0; i < IBUS_MAX_RX_MESSAGES; i++ ) {
        if( msg == &__rx[i].rx ) {
            __rx[i].in_use = false;
            goto done;
        }
    }

done:

    IBUS_MUTEX_UNLOCK();
}

void ibus_message_post( ibus_tx_message_t *msg )
{
    while( pdTRUE != xQueueSendToBack(__ibus_queue, &msg, 0) ) {
        vTaskDelay( 100 );
    }
}


size_t ibus_print( char * text )
{
    ibus_tx_message_t *msg;
    size_t length;
    
    if(    ( NULL == text )
        || ( '\0' == *text ) )
    {
        return 0;
    }
    
    msg = ibus_message_alloc();
    
    strncpy(msg->d.msg_text, text, IBUS_TEXT_LENGTH_MAX+1);
    msg->d.msg_text[IBUS_TEXT_LENGTH_MAX] = '\0';
    msg->type = MT_IBUS__TEXT_MESSAGE;
    length = strlen( msg->d.msg_text );
    ibus_message_post( msg );
    
    return length;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __ibus_task_rx( void *data )
{
    while( 1 ) {
        ibus_io_msg_t *msg;

        msg = ibus_physical_get_message();  /* Blocks thread */
        __process_message( msg );
        ibus_physical_release_message( msg );
    }
}

static void __ibus_task_tx( void *data )
{
    while( 1 ) {
        ibus_tx_message_t *msg;

        _D2( "tx --->\n" );
        xQueueReceive( __ibus_queue, &msg, portMAX_DELAY );
        _D2( "tx <---\n" );

        /* not really needed, but it makes the debug messages easier
         * to separate by ibus.pl */
        vTaskDelay( 200 );
        
        if( NULL == msg ) {
            continue;
        }

        if( MT_IBUS__TX_MESSAGE == msg->type ) {
            const uint8_t *out;
            uint8_t src, dst, out_size;

            src = IBUS_DEVICE_CHANGER;
            dst = IBUS_DEVICE_RADIO;
            out = NULL;
            out_size = 0;

            _D2( "Command: 0x%08x\n", msg->cmd );

            switch( msg->cmd ) {
                case IBUS_TX_CMD__STOPPED:
                case IBUS_TX_CMD__PAUSED:
                case IBUS_TX_CMD__PLAYING:
                case IBUS_TX_CMD__FAST_PLAYING:
                case IBUS_TX_CMD__REWINDING:
                case IBUS_TX_CMD__SEEKING_NEXT:
                case IBUS_TX_CMD__SEEKING_PREV:
                case IBUS_TX_CMD__SEEKING:
                case IBUS_TX_CMD__LOADING_DISC:
                {
                    if( 0 != (IBUS_DISC_STATUS__MAGAZINE_PRESENT & msg->d.status.disc_status) ) {
                        if( 0 != (IBUS_DISC_STATUS__DISC_ANY & msg->d.status.disc_status) ) {
                            uint8_t status[] = { 0x39, 0, 0, 0, 0, 0, 0, 0 };

                            status[1] = (uint8_t) msg->cmd;
                            status[2] = (uint8_t) msg->d.status.audio_state;
                            status[4] = IBUS_DISC_STATUS__DISC_ANY & msg->d.status.disc_status;
                            status[6] = msg->d.status.current_disc;
                            status[7] = msg->d.status.current_track;

                            out = status;
                            out_size = sizeof(status);
                        } else {
                            static const uint8_t no_discs[] = { 0x39, 0x08, 0x02, 0x10, 0, 0, 0, 0 };
                            out = no_discs;
                            out_size = sizeof(no_discs);
                        }
                    } else {
                        static const uint8_t no_mag[] = { 0x39, 0x0a, 0x02, 0x18, 0, 0, 0, 0 };
                        out = no_mag;
                        out_size = sizeof(no_mag);
                    }
                    break;
                }

                case IBUS_TX_CMD__CHECKING_FOR_DISC:
                {
                    uint8_t status[] = { 0x39, 0x09, 0, 0, 0, 0, 0, 0 };
                    status[2] = (uint8_t) msg->d.check_for_disc.audio_state;
                    status[3] = (true == msg->d.check_for_disc.last_failed) ? 0x08 : 0;
                    status[4] = IBUS_DISC_STATUS__DISC_ANY & msg->d.check_for_disc.mask;
                    status[6] = msg->d.check_for_disc.disc;
                    out = status;
                    out_size = sizeof(status);
                    break;
                }

                case IBUS_TX_CMD__ANNOUNCE:
                {
                    static const uint8_t announce[] = { 0x02, 0x01 };
                    out = announce;
                    dst = IBUS_BROADCAST_HIGH;
                    out_size = sizeof(announce);
                    break;
                }

                case IBUS_TX_CMD__POLL_RESPONSE:
                {
                    static const uint8_t poll[] = { 0x02, 0x00 };
                    out = poll;
                    dst = IBUS_BROADCAST_HIGH;
                    out_size = sizeof(poll);
                    break;
                }
            }

            __ibus_send_msg( src, dst, out, out_size );
            ibus_message_free( msg );
        } else if ( MT_IBUS__TEXT_MESSAGE == msg->type ) {
            uint8_t msg_text[IBUS_TEXT_LENGTH_MAX+4];
            printf("Printed to Ibus :: `%s`\n", msg->d.msg_text);
            msg_text[0] = 0x23;
            msg_text[1] = 0x42;
            msg_text[2] = 0x07;
            strcpy( (char*) &(msg_text[3]), msg->d.msg_text );
            __ibus_send_msg( 0xc8, 0x80, msg_text, strlen( msg->d.msg_text )+3 );
            ibus_message_free( msg );
        }
    }
}

static void __process_message( ibus_io_msg_t *msg )
{
    uint8_t *payload;
    uint8_t checksum, src, dst;
    size_t i, length;

    _D2( "%s()\n", __FUNCTION__ );

    if( (NULL == msg) ||
        (IBUS_IO_STATUS__OK != msg->status) ||
        (msg->size < 5) ) /* Min: src|len|dst|payload|chksum */
    {
        return;
    }

    /* See if the message is valid */
    for( checksum = 0, i = 0; i < msg->size; i++ ) {
        checksum ^= msg->buffer[i];
    }

    if( 0 != checksum ) {
        return;
    }

    src = msg->buffer[0];
    length = msg->buffer[1];
    dst = msg->buffer[2];
    payload = &msg->buffer[3];

    if( 0 == length ) {
        return;
    }

    /* We have a valid message... do something with it. */
    if( (IBUS_DEVICE_RADIO == src) &&
        (IBUS_DEVICE_CHANGER == dst) )
    {
        ibus_rx_message_t *rx;
        bool invalid_msg;

        /* Radio -> Device */
        invalid_msg = false;

        rx = __get_rx_message();

        if( (3 == length) && (1 == *payload) ) {
            rx->cmd = IBUS_RX_CMD__POLL;
            if( NULL != __ibus_listener ) {
                _D2( "I sent the message\n" );
                xQueueSendToBack( __ibus_listener, &rx, 0 );
            } else {
                ibus_message_free( rx );
            }
        } else if( (5 == length) && (0x38 == *payload) ) {    /* Radio command */
            payload++;
            rx->cmd = (ibus_rx_cmd_t) *payload++;
            switch( *payload ) {
                case IBUS_RX_CMD__STATUS:
                case IBUS_RX_CMD__STOP:
                case IBUS_RX_CMD__PAUSE:
                case IBUS_RX_CMD__PLAY:
                case IBUS_RX_CMD__FAST_PLAY:
                    break;

                case IBUS_RX_CMD__SEEK:
                    if( (0 == *payload) || (1 == *payload) ) {
                        rx->d.seek = (ibus_direction_t) *payload;
                    } else {
                        invalid_msg = true;
                    }
                    break;

                case IBUS_RX_CMD__CHANGE_DISC:
                    if( 0 != (~IBUS_DISC_STATUS__DISC_ANY & *payload) ) {
                        rx->d.disc = IBUS_DISC_STATUS__DISC_ANY & *payload;
                    } else {
                        invalid_msg = true;
                    }
                    break;

                case IBUS_RX_CMD__SCAN_DISC:
                    if( (0 == *payload) || (1 == *payload) ) {
                        rx->d.scan_disc = (ibus_on_off_t) *payload;
                    } else {
                        invalid_msg = true;
                    }
                    break;

                case IBUS_RX_CMD__RANDOMIZE:
                    if( (0 == *payload) || (1 == *payload) ) {
                        rx->d.randomize = (ibus_on_off_t) *payload;
                    } else {
                        invalid_msg = true;
                    }
                    break;


                default:
                    invalid_msg = true;
                    break;
            }

            if( true == invalid_msg ) {
                ibus_message_free( rx );
                rx = NULL;
            }

            if( NULL != rx ) {
                if( NULL != __ibus_listener ) {
                    xQueueSendToBack( __ibus_listener, &rx, 0 );
                } else {
                    ibus_message_free( rx );
                }
            }
        }
    }
}

ibus_rx_message_t *__get_rx_message( void )
{
    size_t i;

    while( 1 ) {
        IBUS_MUTEX_LOCK();

        for( i = 0; i < IBUS_MAX_RX_MESSAGES; i++ ) {
            if( false == __rx[i].in_use ) {
                __rx[i].in_use = true;
                IBUS_MUTEX_UNLOCK();
                __rx[i].rx.type = MT_IBUS__RX_MESSAGE;
                return &__rx[i].rx;
            }
        }

        IBUS_MUTEX_UNLOCK();

        /* Retry until we succeed. */
        vTaskDelay( 100 );
    }
}

void __ibus_send_msg( const uint8_t src, const uint8_t dst,
                      const uint8_t *msg, const uint8_t length )
{
    size_t i;
    uint8_t checksum;
    uint16_t full_length;
    uint8_t buffer[IBUS_MAX_TX_LENGTH + 4];

    if( (NULL == msg) || (length < 1) || (IBUS_MAX_TX_LENGTH < length)) {
        return;
    }

    buffer[0] = src;
    buffer[1] = length + 2;
    buffer[2] = dst;

    checksum = 0;
    for( i = 0; i < 3; i++ ) {
        checksum ^= buffer[i];
    }
    for( i = 0; i < length; i++ ) {
        buffer[3 + i] = msg[i];
        checksum ^= msg[i];
    }

    buffer[3 + i] = checksum;

    full_length = length + 4;

    ibus_physical_send_message( buffer, full_length );
}
