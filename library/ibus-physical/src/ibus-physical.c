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

#include <bsp/boards/boards.h>
#include <bsp/intc.h>
#include <bsp/pdca.h>
#include <bsp/gpio.h>
#include <bsp/usart.h>

#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "ibus-physical.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define TX_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE)
#define TX_TASK_PRIORITY    (tskIDLE_PRIORITY+1)
#define TX_DELAY            (TASK_DELAY_MS(10))

#define IBUS_PHYSICAL_DEBUG 0

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

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
#define IBUS_PHYSICAL_RX_MSG_MAX    32
#define IBUS_PHYSICAL_TX_MSG_MAX    32

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xQueueHandle __rx_idle;
static xQueueHandle __rx_pending;
static xQueueHandle __tx_idle;
static xQueueHandle __tx_pending;
static xSemaphoreHandle __wake_up_tx_task;
static xSemaphoreHandle __tx_done;
static ibus_io_msg_t __rx_messages[IBUS_PHYSICAL_RX_MSG_MAX];
static ibus_io_msg_t __tx_messages[IBUS_PHYSICAL_TX_MSG_MAX];
static volatile bool __tx_successfully_sent;

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

    __rx_idle = xQueueCreate( IBUS_PHYSICAL_RX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __rx_idle ) {
        goto error_0;
    }
    __rx_pending = xQueueCreate( IBUS_PHYSICAL_RX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __rx_pending ) {
        goto error_1;
    }
    __tx_idle = xQueueCreate( IBUS_PHYSICAL_TX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __tx_idle ) {
        goto error_2;
    }
    __tx_pending = xQueueCreate( IBUS_PHYSICAL_TX_MSG_MAX, sizeof(ibus_io_msg_t*) );
    if( NULL == __tx_pending ) {
        goto error_3;
    }

    for( i = 0; i < IBUS_PHYSICAL_RX_MSG_MAX; i++ ) {
        ibus_io_msg_t *msg = &__rx_messages[i];
        xQueueSendToBack( __rx_idle, &msg, 0 );
    }
    for( i = 0; i < IBUS_PHYSICAL_TX_MSG_MAX; i++ ) {
        ibus_io_msg_t *msg = &__tx_messages[i];
        xQueueSendToBack( __tx_idle, &msg, 0 );
    }

    vSemaphoreCreateBinary( __wake_up_tx_task );
    xSemaphoreTake( __wake_up_tx_task, 0 );
    vSemaphoreCreateBinary( __tx_done );
    xSemaphoreTake( __tx_done, 0 );

    /* ISR for the PDCA/DMA complete */
    intc_register_isr( &__tx_msg_sent,
                       PDCA_GET_ISR_NAME(PDCA_CHANNEL_ID_IBUS_TX),
                       ISR_LEVEL__1 );

    pdca_channel_init( PDCA_CHANNEL_ID_IBUS_TX, IBUS_TX_PDCA_PID, 8 );

    usart_init_rs232( IBUS_USART, &ibus_options );

    xTaskCreate( __tx_task, (signed portCHAR *) "iBusPTX",
                 TX_TASK_STACK_SIZE, NULL, TX_TASK_PRIORITY, NULL );
    return;

error_3:
    vQueueDelete( __tx_idle );
    __tx_idle = NULL;
error_2:
    vQueueDelete( __rx_pending );
    __rx_pending = NULL;
error_1:
    vQueueDelete( __rx_idle );
    __tx_idle = NULL;
error_0:
    return;
}

/* See ibus_physical.h for details. */
void ibus_physical_destroy( void )
{
    _D1( "%s()\n", __func__ );

    if( NULL != __tx_pending ) {
        vQueueDelete( __tx_pending );
        __tx_pending = NULL;
    }

    if( NULL != __tx_idle ) {
        vQueueDelete( __tx_idle );
        __tx_idle = NULL;
    }

    if( NULL != __rx_pending ) {
        vQueueDelete( __rx_pending );
        __rx_pending = NULL;
    }

    if( NULL != __rx_idle ) {
        vQueueDelete( __rx_idle );
        __rx_idle = NULL;
    }
}

/* See ibus_physical.h for details. */
ibus_io_msg_t* ibus_physical_get_message( void )
{
    ibus_io_msg_t *msg;

    _D1( "%s()\n", __func__ );

    xQueueReceive( __rx_pending, &msg, portMAX_DELAY );

    return msg;
}

/* See ibus_physical.h for details. */
void ibus_physical_release_message( ibus_io_msg_t *msg )
{
    _D1( "%s( %p )\n", __func__, (void*) msg );

    if( NULL != msg ) {
        xQueueSendToBack( __rx_idle, &msg, portMAX_DELAY );
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

    xQueueReceive( __tx_idle, &ibus_msg, portMAX_DELAY );

    ibus_msg->status = IBUS_IO_STATUS__OK;
    ibus_msg->size = payload_length + 4;

    ibus_msg->buffer[0] = (uint8_t) src;
    ibus_msg->buffer[1] = (uint8_t) (payload_length + 2);
    ibus_msg->buffer[2] = (uint8_t) dst;

    memcpy( &ibus_msg->buffer[3], payload, payload_length );

    ibus_msg->buffer[payload_length + 3] = (uint8_t) checksum;

    xQueueSendToBack( __tx_pending, &ibus_msg, portMAX_DELAY );
    xSemaphoreGive( __wake_up_tx_task );

    return true;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __tx_task( void *data )
{
    while( 1 ) {
        if( 0 < uxQueueMessagesWaiting(__tx_pending) ) {
            if( true == usart_is_cts(IBUS_USART) ) {
                ibus_io_msg_t *msg;

                /* Start the pending messages now */
                xQueueReceive( __tx_pending, &msg, 0 );
                __tx_successfully_sent = false;

                pdca_queue_buffer( PDCA_CHANNEL_ID_IBUS_TX, msg->buffer, msg->size );
                pdca_isr_enable( PDCA_CHANNEL_ID_IBUS_TX, PDCA_ISR__TRANSFER_COMPLETE );
                pdca_enable( PDCA_CHANNEL_ID_IBUS_TX );

                /* Wait until the message is sent */
                xSemaphoreTake( __tx_done, portMAX_DELAY );

                if( true == __tx_successfully_sent ) {
                    xQueueSendToFront( __tx_idle, &msg, portMAX_DELAY );

                    /* Wait between messages so the radio can have a break. */
                    vTaskDelay( TX_DELAY );
                } else {
                    /* CTS changed state - re-queue & try again. */
                    xQueueSendToFront( __tx_pending, &msg, portMAX_DELAY );
                }
            } else {
                xSemaphoreTake( __wake_up_tx_task, portMAX_DELAY );
            }
        } else {
            xSemaphoreTake( __wake_up_tx_task, portMAX_DELAY );
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
        portBASE_TYPE yield;

        /* get a message, populate it, and send it, or drop the message. */
        if( pdTRUE == xQueueReceiveFromISR(__rx_idle, &msg, &yield) ) {
            msg->status = status;
            msg->size = i;
            memcpy( msg->buffer, buffer, i );
            xQueueSendToBackFromISR( __rx_pending, &msg, &yield );
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
    portBASE_TYPE ignore;

    if( true == cts ) {
        /* Enable starting a new transfer */
        xSemaphoreGiveFromISR( __wake_up_tx_task, &ignore );
    } else {
        /* Cancel the active transfer */
        pdca_disable( PDCA_CHANNEL_ID_IBUS_TX );
        pdca_isr_disable( PDCA_CHANNEL_ID_IBUS_TX, PDCA_ISR__TRANSFER_COMPLETE );

        xSemaphoreGiveFromISR( __tx_done, &ignore );
    }
}

__attribute__((__interrupt__))
static void __tx_msg_sent( void )
{
    portBASE_TYPE ignore;

    pdca_disable( PDCA_CHANNEL_ID_IBUS_TX );
    pdca_isr_disable( PDCA_CHANNEL_ID_IBUS_TX, PDCA_ISR__TRANSFER_COMPLETE );

    __tx_successfully_sent = true;
    xSemaphoreGiveFromISR( __tx_done, &ignore );
}
