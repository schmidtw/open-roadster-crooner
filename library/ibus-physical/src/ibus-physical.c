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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <bsp/boards/boards.h>
#include <bsp/intc.h>
#include <bsp/pdca.h>
#include <bsp/gpio.h>
#include <bsp/usart.h>

#include <freertos/os.h>

#include "ibus-physical.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define TX_TASK_STACK_SIZE  (120)
#define TX_TASK_PRIORITY    (1)
#define TX_DELAY            (10)

#define IBUS_PHYSICAL_DEBUG     0
#define IBUS_LOG_FLAG           0
#define IBUS_LOG_MSGS_PER_WRITE 5

#define _D1(...)
#define _D2(...)

#if (defined(IBUS_PHYSICAL_DEBUG) && (0 < IBUS_PHYSICAL_DEBUG))
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (defined(IBUS_PHYSICAL_DEBUG) && (1 < IBUS_PHYSICAL_DEBUG))
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

#define _IBUS_LOG(x)
#if (0 < IBUS_LOG_FLAG)
#undef _IBUS_LOG
#define _IBUS_LOG(x) __out( x )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
#define IBUS_PHYSICAL_RX_MSG_MAX    32
#define IBUS_PHYSICAL_TX_MSG_MAX    32

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static queue_handle_t __rx_idle;
static queue_handle_t __rx_pending;
static queue_handle_t __tx_idle;
static queue_handle_t __tx_pending;
static semaphore_handle_t __wake_up_tx_task;
static semaphore_handle_t __tx_done;
static ibus_io_msg_t __rx_messages[IBUS_PHYSICAL_RX_MSG_MAX];
static ibus_io_msg_t __tx_messages[IBUS_PHYSICAL_TX_MSG_MAX];
static volatile bool __tx_successfully_sent;
static void __out( ibus_io_msg_t *msg );

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __tx_task( void *data );
static void __get_char_isr( const int c );
static void __cts_change_isr( const bool cts );
__attribute__((__interrupt__))
static void __tx_msg_sent( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See ibus_physical.h for details. */
void ibus_physical_init( void )
{
    static const gpio_map_t ibus_pins_map[] =
    {
        { .pin = IBUS_RX_PIN,  .function = IBUS_RX_FUNCTION  },
        { .pin = IBUS_TX_PIN,  .function = IBUS_TX_FUNCTION  },
        { .pin = IBUS_CTS_PIN, .function = IBUS_CTS_FUNCTION }
    };

    /* These are the iBus settings. */
    static const usart_options_t ibus_options = {
        .baudrate     = 9600,
        .data_bits    = USART_DATA_BITS__8,
        .parity       = USART_PARITY__EVEN,
        .stop_bits    = USART_STOPBITS__1,
        .mode         = USART_MODE__NORMAL,
        .hw_handshake = true,
        .map_size     = sizeof(ibus_pins_map) / sizeof(gpio_map_t),
        .map          = ibus_pins_map,
        .dir          = USART_DIRECTION__BOTH,

        .new_char_fn  = __get_char_isr,
        .timeout_us   = 1500,
        .periodic     = false,

        .cts_fn       = __cts_change_isr
    };

    int i;

    _D1( "%s()\n", __func__ );

    __rx_idle = os_queue_create( IBUS_PHYSICAL_RX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __rx_idle ) {
        goto error_0;
    }
    __rx_pending = os_queue_create( IBUS_PHYSICAL_RX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __rx_pending ) {
        goto error_1;
    }
    __tx_idle = os_queue_create( IBUS_PHYSICAL_TX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __tx_idle ) {
        goto error_2;
    }
    __tx_pending = os_queue_create( IBUS_PHYSICAL_TX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __tx_pending ) {
        goto error_3;
    }

    for( i = 0; i < IBUS_PHYSICAL_RX_MSG_MAX; i++ ) {
        ibus_io_msg_t *msg = &__rx_messages[i];
        os_queue_send_to_back( __rx_idle, &msg, NO_WAIT );
    }
    for( i = 0; i < IBUS_PHYSICAL_TX_MSG_MAX; i++ ) {
        ibus_io_msg_t *msg = &__tx_messages[i];
        os_queue_send_to_back( __tx_idle, &msg, NO_WAIT );
    }

     __wake_up_tx_task = os_semaphore_create_binary();
    os_semaphore_take( __wake_up_tx_task, NO_WAIT );
     __tx_done = os_semaphore_create_binary();
    os_semaphore_take( __tx_done, NO_WAIT );

    /* ISR for the PDCA/DMA complete */
    intc_register_isr( &__tx_msg_sent,
                       PDCA_GET_ISR_NAME(PDCA_CHANNEL_ID_IBUS_TX),
                       ISR_LEVEL__1 );

    pdca_channel_init( PDCA_CHANNEL_ID_IBUS_TX, IBUS_TX_PDCA_PID, 8 );

    usart_init_rs232( IBUS_USART, &ibus_options );

    os_task_create( __tx_task, "iBusPTX", TX_TASK_STACK_SIZE,
                    NULL, TX_TASK_PRIORITY, NULL );
    return;

error_3:
    os_queue_delete( __tx_idle );
    __tx_idle = NULL;
error_2:
    os_queue_delete( __rx_pending );
    __rx_pending = NULL;
error_1:
    os_queue_delete( __rx_idle );
    __tx_idle = NULL;
error_0:
    return;
}

/* See ibus_physical.h for details. */
void ibus_physical_destroy( void )
{
    _D1( "%s()\n", __func__ );

    if( NULL != __tx_pending ) {
        os_queue_delete( __tx_pending );
        __tx_pending = NULL;
    }

    if( NULL != __tx_idle ) {
        os_queue_delete( __tx_idle );
        __tx_idle = NULL;
    }

    if( NULL != __rx_pending ) {
        os_queue_delete( __rx_pending );
        __rx_pending = NULL;
    }

    if( NULL != __rx_idle ) {
        os_queue_delete( __rx_idle );
        __rx_idle = NULL;
    }
}

/* See ibus_physical.h for details. */
ibus_io_msg_t* ibus_physical_get_message( void )
{
    ibus_io_msg_t *msg;

    _D1( "%s()\n", __func__ );

    os_queue_receive( __rx_pending, &msg, WAIT_FOREVER );

    _IBUS_LOG( msg );

    return msg;
}

/* See ibus_physical.h for details. */
void ibus_physical_release_message( ibus_io_msg_t *msg )
{
    _D1( "%s( %p )\n", __func__, (void*) msg );

    if( NULL != msg ) {
        os_queue_send_to_back( __rx_idle, &msg, WAIT_FOREVER );
    }
}

/* See ibus_physical.h for details. */
bool ibus_physical_send_message( const ibus_device_t src,
                                 const ibus_device_t dst,
                                 const uint8_t *payload,
                                 const size_t payload_length )
{
    ibus_io_msg_t *ibus_msg;
    int32_t i;
    uint8_t checksum;

    if( (NULL == payload) ||
        (0 == payload_length) ||
        (IBUS_MAX_MESSAGE_SIZE < (payload_length + 4)) )
    {
        return false;
    }

    checksum = 0;
    checksum ^= (uint8_t) src;
    checksum ^= (uint8_t) dst;
    checksum ^= (uint8_t) (2 + payload_length);
    for( i = 0; i < payload_length; i++ ) {
        checksum ^= payload[i];
    }

    os_queue_receive( __tx_idle, &ibus_msg, WAIT_FOREVER );

    ibus_msg->status = IBUS_IO_STATUS__OK;
    ibus_msg->size = payload_length + 4;

    ibus_msg->buffer[0] = (uint8_t) src;
    ibus_msg->buffer[1] = (uint8_t) (payload_length + 2);
    ibus_msg->buffer[2] = (uint8_t) dst;

    memcpy( &ibus_msg->buffer[3], payload, payload_length );

    ibus_msg->buffer[payload_length + 3] = (uint8_t) checksum;

    _IBUS_LOG( ibus_msg );

    os_queue_send_to_back( __tx_pending, &ibus_msg, WAIT_FOREVER );
    os_semaphore_give( __wake_up_tx_task );

    return true;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __tx_task( void *data )
{
    while( 1 ) {
        if( 0 < os_queue_get_queued_messages_waiting(__tx_pending) ) {
            if( true == usart_is_cts(IBUS_USART) ) {
                ibus_io_msg_t *msg;

                /* Start the pending messages now */
                os_queue_receive( __tx_pending, &msg, NO_WAIT );
                __tx_successfully_sent = false;

                pdca_queue_buffer( PDCA_CHANNEL_ID_IBUS_TX, msg->buffer, msg->size );
                pdca_isr_enable( PDCA_CHANNEL_ID_IBUS_TX, PDCA_ISR__TRANSFER_COMPLETE );
                pdca_enable( PDCA_CHANNEL_ID_IBUS_TX );

                /* Wait until the message is sent */
                os_semaphore_take( __tx_done, WAIT_FOREVER );

                if( true == __tx_successfully_sent ) {
                    os_queue_send_to_front( __tx_idle, &msg, WAIT_FOREVER );

                    /* Wait between messages so the radio can have a break. */
                    os_task_delay_ms( TX_DELAY );
                } else {
                    /* CTS changed state - re-queue & try again. */
                    os_queue_send_to_front( __tx_pending, &msg, WAIT_FOREVER );
                }
            } else {
                os_semaphore_take( __wake_up_tx_task, WAIT_FOREVER );
            }
        } else {
            os_semaphore_take( __wake_up_tx_task, WAIT_FOREVER );
        }
    }
}

static void __get_char_isr( const int c )
{
    static ibus_io_msg_status_t status = IBUS_IO_STATUS__OK;
    static uint8_t buffer[IBUS_MAX_MESSAGE_SIZE];
    static int i = 0;

    _D2( "%s()\n", __func__ );

    if( -1 == c ) {
        ibus_io_msg_t *msg;

        /* get a message, populate it, and send it, or drop the message. */
        if( true == os_queue_receive_ISR(__rx_idle, &msg, NULL) ) {
            msg->status = status;
            msg->size = i;
            memcpy( msg->buffer, buffer, i );
            os_queue_send_to_back_ISR( __rx_pending, &msg, NULL );
        }

        /* reset our state */
        i = 0;
        status = IBUS_IO_STATUS__OK;
    } else if( -2 == c ) {
        status = IBUS_IO_STATUS__PARITY_ERROR;
    } else {
        if( IBUS_MAX_MESSAGE_SIZE < i ) {
            status = IBUS_IO_STATUS__BUFFER_OVERRUN_ERROR;
            i = 0;
        }

        buffer[i++] = (char) c;
    }
}

static void __cts_change_isr( const bool cts )
{
    if( true == cts ) {
        /* Enable starting a new transfer */
        os_semaphore_give_ISR( __wake_up_tx_task, NULL );
    } else {
        /* Cancel the active transfer */
        pdca_disable( PDCA_CHANNEL_ID_IBUS_TX );
        pdca_isr_disable( PDCA_CHANNEL_ID_IBUS_TX, PDCA_ISR__TRANSFER_COMPLETE );

        os_semaphore_give_ISR( __tx_done, NULL );
    }
}

__attribute__((__interrupt__))
static void __tx_msg_sent( void )
{
    pdca_disable( PDCA_CHANNEL_ID_IBUS_TX );
    pdca_isr_disable( PDCA_CHANNEL_ID_IBUS_TX, PDCA_ISR__TRANSFER_COMPLETE );

    __tx_successfully_sent = true;
    os_semaphore_give_ISR( __tx_done, NULL );
}

static void __out( ibus_io_msg_t *msg )
{
    struct timeval tv;
    struct timezone tz;
    static FILE *__file = NULL;
    static char tmp[255];
    int offset;
    int i;
    static int count = 0;

    gettimeofday( &tv, &tz );
    offset = sprintf( tmp, "[%u.%06u] Status: %s Length: %u [%02x|%02x|%02x]",
             (unsigned int) tv.tv_sec, (unsigned int) tv.tv_usec, (IBUS_IO_STATUS__OK == msg->status) ? "OK" : (IBUS_IO_STATUS__PARITY_ERROR == msg->status) ? "PE" : "BOR", (unsigned int) msg->size, msg->buffer[0], msg->buffer[1], msg->buffer[2] );
    for( i = 3; i < msg->size; i++ ) {
        offset += sprintf( &tmp[offset], " %02x", msg->buffer[i] );
    }
    tmp[offset] = '\0';

    puts( tmp );

    tmp[offset++] = '\n';
    tmp[offset] = '\0';

    if( NULL == __file ) {
        __file = fopen( "/ibus-log.txt", "a" );
    }

    if( NULL != __file ) {
        fputs( tmp, __file );
        count++;
        if( IBUS_LOG_MSGS_PER_WRITE <= count ) {
            fflush( __file );
            fclose( __file );
            __file = NULL;
            count = 0;
        }
    }
}
