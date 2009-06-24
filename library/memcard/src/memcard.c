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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <avr32/io.h>

#include <bsp/boards/boards.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/intc.h>
#include <bsp/spi.h>
#include <bsp/pdca.h>

#include <freertos/portmacro.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "config.h"
#include "memcard.h"
#include "crc.h"
#include "commands.h"
#include "command.h"
#include "timing-parameters.h"
#include "io.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define AUTOMOUNT_STACK_SIZE    (configMINIMAL_STACK_SIZE + 1400)
#define AUTOMOUNT_PRIORITY      (tskIDLE_PRIORITY+1)
#define AUTOMOUNT_CALLBACK_MAX  3

#define MC_RX_ISR       PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_RX )
#define MC_TX_ISR       PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_TX )

#define MC_BLOCK_START  0xFE

#define MC_READ_SINGLE_BLOCK 17
#define MC_COMMAND_BUFFER_SIZE  (MC_Ncs + 6 + MC_Ncr)
#define MC_BLOCK_BUFFER_SIZE    (512 + 10)

#define MIN( a, b )     ((a) < (b)) ? (a) : (b)


#define __MC_CSR( index )   MC_SPI->CSR##index
#define MC_CSR( index )     __MC_CSR( index )

#define _D1(...)
#define _D2(...)

#ifdef MEMCARD_DEBUG
#if (0 < MEMCARD_DEBUG)
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif
#if (1 < MEMCARD_DEBUG)
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum { MCT_UNKNOWN = 0x00,
               MCT_MMC     = 0x10,
               MCT_SD      = 0x20,
               MCT_SD_20   = 0x21,
               MCT_SDHC    = 0x22 } mc_card_type_t;

typedef enum {
    MCS_NO_CARD,
    MCS_CARD_DETECTED,
    MCS_POWERED_ON,
    MCS_ASSERT_SPI_MODE,
    MCS_INTERFACE_CONDITION_CHECK,
    MCS_INTERFACE_CONDITION_SUCCESS,
    MCS_INTERFACE_CONDITION_FAILURE,
    MCS_SD10_COMPATIBLE_VOLTAGE,
    MCS_SD20_COMPATIBLE_VOLTAGE,
    MCS_SD10_CARD_READY,
    MCS_SD20_CARD_READY,
    MCS_MMC_CARD,
    MCS_DETERMINE_METRICS,
    MCS_SET_BLOCK_SIZE,
    MCS_CARD_READY,
    MCS_CARD_UNUSABLE
} mc_mount_state_t;

typedef enum {
    MRS_QUEUED,
    MRS_COMMAND_SENT,
    MRS_LOOKING_FOR_BLOCK,
    MRS_BLOCK_START_FOUND,
    MRS_SUCCESS,
    MRS_TIMEOUT
} mc_request_state_t;

typedef struct {
    uint8_t command[MC_COMMAND_BUFFER_SIZE];
    uint8_t *buffer;
    uint32_t length;
    uint8_t crc[2];
    uint32_t crc_length;
    mc_request_state_t state;
    uint32_t nac;
} mc_message_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static mc_card_type_t mc_type;
static uint64_t mc_size;
static uint32_t mc_Nac_read;
static uint32_t mc_block_size;  /**< In bytes. */

static xQueueHandle __idle;
static xQueueHandle __pending;
static xQueueHandle __complete;
static xSemaphoreHandle __card_state_change;
static mc_message_t __messages[MC_MSG_MAX];
static uint8_t *__command_buffer;
static uint8_t *__block_buffer;
static volatile mc_card_status_t __card_status;
static card_status_fct __callback_fns[AUTOMOUNT_CALLBACK_MAX];

/*----------------------------------------------------------------------------*/
/*                                  Constants                                 */
/*----------------------------------------------------------------------------*/
#if (522 != MC_BLOCK_BUFFER_SIZE)
#error Need to update the __dummy_data
#endif
static const uint8_t __dummy_data[MC_BLOCK_BUFFER_SIZE] =
{
    /* 512 */
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,

    /* 10 */
    0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,  0xff, 0xff
};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static mc_status_t __send_card_to_idle_state( void );
static mc_status_t __is_card_voltage_compat( void );
static void __fast_read_single_block( mc_message_t *msg );
static void __fast_process_block( void );
static mc_status_t __determine_metrics( void );
static mc_status_t __set_block_size( void );
__attribute__((__interrupt__))
static void __card_change( void );
__attribute__((__interrupt__))
static void __fast_read_handler( void );
static inline void __queue_and_send( const uint8_t *send,
                                     uint8_t *receive,
                                     const size_t length,
                                     const bool select );
static inline uint32_t __find_and_process_block_start( mc_message_t *msg,
                                                       const uint8_t *start,
                                                       const size_t len );
static void __automount_task( void *data );
static mc_status_t __mc_mount( void );
static void __call_all( const mc_card_status_t status );
static bool __card_present( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See memcard.h for details. */
mc_status_t mc_init( void* (*fast_malloc_fn)(size_t) )
{
    int32_t i;

    __card_status = MC_CARD__REMOVED;
    vSemaphoreCreateBinary( __card_state_change );
    xSemaphoreTake( __card_state_change, 0 );

    for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
        __callback_fns[i] = NULL;
    }

    __idle = xQueueCreate( MC_MSG_MAX, sizeof(mc_message_t*) );
    __pending = xQueueCreate( MC_MSG_MAX, sizeof(mc_message_t*) );
    __complete = xQueueCreate( MC_MSG_MAX, sizeof(mc_message_t*) );

    __command_buffer = (uint8_t *) (*fast_malloc_fn)( MC_COMMAND_BUFFER_SIZE );
    __block_buffer = (uint8_t *) (*fast_malloc_fn)( MC_BLOCK_BUFFER_SIZE );

    for( i = 0; i < MC_MSG_MAX; i++ ) {
        mc_message_t *msg = &__messages[i];
        xQueueSendToBack( __idle, &msg, 0 );
    }

    intc_register_isr( &__fast_read_handler, MC_RX_ISR, ISR_LEVEL__1 );

    pdca_channel_init( PDCA_CHANNEL_ID_MC_RX, MC_PDCA_RX_PERIPHERAL_ID, 8 );
    pdca_channel_init( PDCA_CHANNEL_ID_MC_TX, MC_PDCA_TX_PERIPHERAL_ID, 8 );

    xTaskCreate( __automount_task, (signed portCHAR *) "AMNT",
                 AUTOMOUNT_STACK_SIZE, NULL, AUTOMOUNT_PRIORITY, NULL );

    return MC_RETURN_OK;
}

/* See memcard.h for details. */
mc_status_t mc_register( card_status_fct card_status_fn )
{
    int32_t i;

    if( NULL == card_status_fn ) {
        return MC_ERROR_PARAMETER;
    }

    for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
        if( NULL == __callback_fns[i] ) {
            __callback_fns[i] = card_status_fn;
            return MC_RETURN_OK;
        }
    }

    return MC_TOO_MANY_REGISTERED;
}

mc_status_t mc_cancel( card_status_fct card_status_fn )
{
    if( NULL != card_status_fn ) {
        int32_t i;

        for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
            if( card_status_fn == __callback_fns[i] ) {
                __callback_fns[i] = NULL;
                return MC_RETURN_OK;
            }
        }
    }

    return MC_ERROR_PARAMETER;
}

/* See memcard.h for details. */
mc_card_status_t mc_get_status( void )
{
    return __card_status;
}

/* See memcard.h for details. */
mc_status_t mc_read_block( const uint32_t lba, uint8_t *buffer )
{
    mc_status_t status;
    uint32_t address = lba;
    mc_message_t *msg;
    int32_t i;

#if (0 < STRICT_PARAMS)
    if( NULL == buffer ) {
        return MC_ERROR_PARAMETER;
    }
#endif

    if( MC_CARD__MOUNTED != __card_status ) {
        return MC_NOT_MOUNTED;
    }

    if( mc_type != MCT_SDHC ) {
        address <<= 9;
    }

    xQueueReceive( __idle, &msg, portMAX_DELAY );

    /* Fill MC_Ncs bytes worth of data with 0xff.
     * This is needed for the timing. */
    for( i = 0; i < MC_Ncs; i++ ) {
        msg->command[i] = 0xff;
    }

    /* Calculate the command sequence */
    msg->command[i++] = 0x40 | (0x3f & MC_READ_SINGLE_BLOCK);
    msg->command[i++] = address >> 24;
    msg->command[i++] = 0xff & (address >> 16);
    msg->command[i++] = 0xff & (address >> 8);
    msg->command[i++] = 0xff & address;
    msg->command[i++] = (crc7(&msg->command[MC_Ncs], 5) << 1) | 0x01;

    memset( &msg->command[i], 0xff, (MC_COMMAND_BUFFER_SIZE - i) );

    msg->buffer = buffer;
    msg->length = 0;
    msg->state = MRS_QUEUED;
    msg->nac = 0;
    msg->crc_length = 0;

    _D2( "Getting ready to read...\n" );
    taskENTER_CRITICAL();
    {
        /* Need to use ISR versions since we're in a critcal section. */
        portBASE_TYPE yield;
        xQueueSendToBackFromISR( __pending, &msg, &yield );

        if( 1 == uxQueueMessagesWaitingFromISR(__pending) ) {
            __fast_read_single_block( msg );
        }

    }
    taskEXIT_CRITICAL();

    _D2( "Waiting...\n" );
    xQueueReceive( __complete, &msg, portMAX_DELAY );
    status = (MRS_SUCCESS == msg->state) ? MC_RETURN_OK : MC_ERROR_TIMEOUT;
    xQueueSendToBack( __idle, &msg, 0 );

    _D2( "Got response: 0x%04x\n", status );
    return status;
}

/* See memcard.h for details. */
mc_status_t mc_write_block( const uint32_t lba, const uint8_t *buffer )
{
    if( MC_CARD__MOUNTED != __card_status ) {
        return MC_NOT_MOUNTED;
    }

    return MC_NOT_SUPPORTED;
}

/* See memcard.h for details. */
mc_status_t mc_get_block_count( uint32_t *blocks )
{

#if (0 < STRICT_PARAMS)
    if( NULL == blocks ) {
        return MC_ERROR_PARAMETER;
    }
#endif

    if( MC_CARD__MOUNTED != __card_status ) {
        return MC_NOT_MOUNTED;
    }

    *blocks = (uint32_t) (mc_size / 512ULL);

    return MC_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static mc_status_t __send_card_to_idle_state( void )
{
    int32_t i;

    for( i = 0; i < 10; i++ ) {
        if( MC_RETURN_OK == mc_go_idle_state() ) {
            return MC_RETURN_OK;
        }
    }

    return MC_INIT_ERROR;
}

static mc_status_t __is_card_voltage_compat( void )
{
    return MC_RETURN_OK;
}

static void __fast_read_single_block( mc_message_t *msg )
{
    memcpy( __command_buffer, msg->command, MC_COMMAND_BUFFER_SIZE );

    __queue_and_send( __command_buffer, __command_buffer,
                      MC_COMMAND_BUFFER_SIZE, true );

    msg->state = MRS_COMMAND_SENT;
}

static void __fast_process_block( void )
{
    portBASE_TYPE yield;
    mc_message_t *msg;

process_next:

    taskENTER_CRITICAL();
    if( pdTRUE == xQueueReceiveFromISR(__pending, &msg, &yield) ) {
        bool requeue;

        requeue = false;

        if( MRS_QUEUED == msg->state ) {
            _D2( "MRS_QUEUED\n" );
            __fast_read_single_block( msg );
            msg->state = MRS_COMMAND_SENT;
            requeue = true;
        } else if( MRS_COMMAND_SENT == msg->state ) {
            uint8_t *r1, *end;

            _D2( "MRS_COMMAND_SENT\n" );
            end = &__command_buffer[MC_COMMAND_BUFFER_SIZE];

            r1 = memchr( __command_buffer, 0, MC_COMMAND_BUFFER_SIZE );
            if( NULL == r1 ) {
                msg->state = MRS_TIMEOUT;
                /* We're done... */
                _D2( "Command TIMEOUT\n" );
            } else {
                msg->state = MRS_LOOKING_FOR_BLOCK;

                __find_and_process_block_start( msg, r1, (end - r1) );
                requeue = true;

                __queue_and_send( __dummy_data, __block_buffer,
                                  MC_BLOCK_BUFFER_SIZE, false );
            }
        } else if( MRS_LOOKING_FOR_BLOCK == msg->state ) {
            uint32_t left;
            uint32_t send;
            bool done;

            _D2( "MRS_LOOKING_FOR_BLOCK\n" );
            send = MC_BLOCK_BUFFER_SIZE;
            left = __find_and_process_block_start( msg, __block_buffer,
                                                   MC_BLOCK_BUFFER_SIZE );

            done = false;
            if( MRS_LOOKING_FOR_BLOCK == msg->state ) {
                _D2( "Still MRS_LOOKING_FOR_BLOCK\n" );
                if( mc_Nac_read < msg->nac ) {
                    _D2( "NAC Timeout.\n" );
                    msg->state = MRS_TIMEOUT;
                    done = true;
                }
            } else {
                _D2( "found block\n" );
                if( (msg->length < 512) || (msg->crc_length < 2) ) {
                    send = 512 - msg->length;
                    send += 2 - msg->crc_length;
                } else {
                    msg->state = MRS_SUCCESS;
                    _D2( "Got entire block\n" );
                    done = true;
                }
            }

            if( false == done ) {
                requeue = true;
                __queue_and_send( __dummy_data, __block_buffer, send, false );
            }
        } else if( MRS_BLOCK_START_FOUND == msg->state ) {
            _D2( "MRS_BLOCK_START_FOUND\n" );
            if( msg->length < 512 ) {
                uint32_t copy_length;

                copy_length = 512 - msg->length;

                /* Need to copy more of the message */
                memcpy( &msg->buffer[msg->length], __block_buffer, copy_length );
                msg->length += copy_length;
            
                msg->crc[0] = __block_buffer[copy_length];
                msg->crc[1] = __block_buffer[copy_length + 1];
                msg->crc_length = 2;
            }

            msg->state = MRS_SUCCESS;
            /* We're done... */
        }

        if( true == requeue ) {
            /* Re-queue the message at the front. */
            xQueueSendToFrontFromISR( __pending, &msg, &yield );
        } else {
            io_unselect();
            xQueueSendToBackFromISR( __complete, &msg, &yield );
            _D2( "Goto process_next\n" );
            taskEXIT_CRITICAL();
            goto process_next;
        }
    }
    taskEXIT_CRITICAL();
}

static mc_status_t __determine_metrics( void )
{
    mc_csd_t csd;
    uint32_t pba_hz;
    mc_status_t status;
    uint32_t baud_rate;

    pba_hz = pm_get_frequency( PM__PBA );

    status = mc_get_csd( &csd, pba_hz );
    if( MC_RETURN_OK != status ) {
        return status;
    }

    if( (0 == csd.max_speed) ||
        (0 == csd.total_size) ||
        (0 == csd.block_size) )
    {
        return MC_UNUSABLE;
    }

    baud_rate = csd.max_speed;
    mc_block_size = csd.block_size;
    mc_size = csd.total_size;
    mc_Nac_read = csd.nac_read;

    spi_set_baudrate( MC_SPI, MC_CS, baud_rate );
    spi_get_baudrate( MC_SPI, MC_CS, &baud_rate );
    _D2( "Actual baud rate: %lu\n", baud_rate );

    return MC_RETURN_OK;
}


static mc_status_t __set_block_size( void )
{
    mc_status_t status;

    /* Only set this if we need to - some cards don't like
     * being set when they are already set to 512 byte blocks. */
    if( 512 == mc_block_size ) {
        return MC_RETURN_OK;
    }

    status = mc_set_block_size( 512 );
    if( MC_RETURN_OK == status ) {
        return MC_RETURN_OK;
    }

    return MC_INIT_ERROR;
}

__attribute__((__interrupt__))
static void __card_change( void )
{
    portBASE_TYPE ignore;
    
    if( false == __card_present() ) {
        /* Force clean up now so we're ready for a new card insertion. */
        mc_type = MCT_UNKNOWN;
        gpio_reset_pin( MC_SCK_PIN );
        gpio_reset_pin( MC_MISO_PIN );
        gpio_reset_pin( MC_MOSI_PIN );
        gpio_reset_pin( MC_CS_PIN );
        io_unselect();
    }

    xSemaphoreGiveFromISR( __card_state_change, &ignore );

    gpio_clr_interrupt_flag( MC_CD_PIN );
}

__attribute__((__interrupt__))
static void __fast_read_handler( void )
{
    volatile avr32_pdca_channel_t *tx, *rx;

    tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];
    rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];

    /* Disable 'transfer complete' */
    tx->idr = AVR32_PDCA_TRC_MASK;
    rx->idr = AVR32_PDCA_TRC_MASK;

    /* Clear the ISR values */
    tx->isr;
    rx->isr;

    /* Disable 'tx' pdca channel */
    tx->cr = AVR32_PDCA_CR_TDIS_MASK;

    /* Disable 'rx' pdca channel */
    rx->cr = AVR32_PDCA_CR_TDIS_MASK;

    __fast_process_block();
}

static inline void __queue_and_send( const uint8_t *send,
                                     uint8_t *receive,
                                     const size_t length,
                                     const bool select )
{
    volatile avr32_pdca_channel_t *rx, *tx;
    uint8_t s0;

    s0 = send[0];

    pdca_queue_buffer( PDCA_CHANNEL_ID_MC_RX, receive, length );

    /* Q: Why (length - 1)?
     * A: Because we do an extra write to start with, the
     * DMA controller does this: [R][W]......[R][W][R]  Since
     * we start and end with reading a byte, the writes must
     * be 1 less.
     */
    pdca_queue_buffer( PDCA_CHANNEL_ID_MC_TX, (uint8_t*) &send[1], (length - 1) );

    if( true == select ) {
        io_select();
    }

    io_send( s0 );

    rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];
    tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];

    /* Enable the transfer complete ISR */
    rx->ier = AVR32_PDCA_TRC_MASK;

    /* Enable the transfer. */
    rx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;
    tx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;

    rx->isr;
    tx->isr;
}

static inline uint32_t __find_and_process_block_start( mc_message_t *msg,
                                                       const uint8_t *start,
                                                       const size_t len )
{
    uint8_t *block_start;
    uint32_t left;

    _D2( "%s( msg, start, %d )\n", __func__, len );

    left = len;

    block_start = memchr( start, MC_BLOCK_START, len );

    if( NULL == block_start ) {
        msg->nac += len;
        left -= len;
    } else {
        uint32_t nac;

        nac = (block_start - start);
        msg->nac += nac;
        left -= nac;

        if( 0 < left ) {
            left--;
            block_start++;

            memcpy( msg->buffer, block_start, MIN(512, left) );

            msg->length = MIN( 512, left );
            left -= msg->length;
        }
        if( 0 < left ) {
            msg->crc[0] = block_start[512];
            left--;
            msg->crc_length++;
            if( 0 < left ) {
                msg->crc[1] = block_start[513];
                left--;
                msg->crc_length++;
            }
        }
        msg->state = MRS_BLOCK_START_FOUND;
    }

    _D2( "left: %lu\n", left );
    return left;
}

static void __automount_task( void *data )
{
    /* Create an interrupt on the card ejection. */
    gpio_set_options( MC_CD_PIN,
                      GPIO_DIRECTION__INPUT,
                      GPIO_PULL_UP__DISABLE,
                      GPIO_GLITCH_FILTER__ENABLE,
                      GPIO_INTERRUPT__CHANGE,
                      0 );

    intc_register_isr( &__card_change, MC_CD_ISR, ISR_LEVEL__2 );

    /* In case we booted with a card in the slot. */
    if( true == __card_present() ) {
        xSemaphoreGive( __card_state_change );
    }

    while( 1 ) {
        xSemaphoreTake( __card_state_change, portMAX_DELAY );

        if( true == __card_present() ) {
            __card_status = MC_CARD__INSERTED;
            __call_all( __card_status );
        } else {
            __card_status = MC_CARD__REMOVED;
            __call_all( __card_status );
        }

        if( MC_CARD__INSERTED == __card_status ) {
            mc_status_t status;

            status = __mc_mount();
            if( MC_RETURN_OK == status ) {
                __card_status = MC_CARD__MOUNTED;
                __call_all( __card_status );
            } else if( MC_UNUSABLE == status ) {
                __card_status = MC_CARD__UNUSABLE;
                __call_all( __card_status );
            } else {
                /* The card may have been removed during initialization. */
                if( true == __card_present() ) {
                    /* Nope, just can't use the card. */
                    __card_status = MC_CARD__UNUSABLE;
                    __call_all( __card_status );
                }
            }
        }
    }
}

/**
 *  Used to mount a memory card when present in the defined hardware slot.
 *
 *  @return Status.
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_INIT_ERROR       The card did not respond properly.
 *      @retval MC_UNUSABLE         The card is not compatible.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_CRC_FAILURE      The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 */
static mc_status_t __mc_mount( void )
{
    mc_mount_state_t mount_state;
    int retries;
    static const gpio_map_t map[] = { { MC_SCK_PIN,  MC_SCK_FUNCTION  },
                                      { MC_MISO_PIN, MC_MISO_FUNCTION },
                                      { MC_MOSI_PIN, MC_MOSI_FUNCTION },
                                      { MC_CS_PIN,   MC_CS_FUNCTION   } };

    retries = 10;

    /* Initialize the hardware. */
    gpio_enable_module( map, sizeof(map)/sizeof(gpio_map_t) );

    spi_reset( MC_SPI );

    MC_SPI->MR.mstr = 1;    /* master mode */
    MC_SPI->MR.modfdis = 1; /* ignore faults */
    MC_SPI->MR.dlybcs = 8;  /* make sure there is a delay between CSs */

    if( MC_RETURN_OK !=
            spi_set_baudrate(MC_SPI, MC_CS, MC_BAUDRATE_INITIALIZATION) )
    {
        return MC_INIT_ERROR;
    }

    /* Allow 8 clock cycles of up time for the chip select
     * prior to enabling/disabling it */
    (MC_CSR(MC_CS)).dlybs  = 8;

    /* We need a small delay between bytes, otherwise we seem to
     * get data corruption unless we are going really slow. */
    (MC_CSR(MC_CS)).dlybct = 1;

    /* scbr is set by spi_set_baudrate() */

    (MC_CSR(MC_CS)).bits   = 0;
    (MC_CSR(MC_CS)).csaat  = 1;
    (MC_CSR(MC_CS)).csnaat = 0;
    (MC_CSR(MC_CS)).ncpha  = 1;
    (MC_CSR(MC_CS)).cpol   = 0;

    mount_state = MCS_NO_CARD;

    while( 1 ) {
        switch( mount_state ) {
            case MCS_NO_CARD:
                _D2( "MCS_NO_CARD:\n" );
                mc_type = MCT_UNKNOWN;
                if( true == __card_present() ) {
                    _D2( "MCS_NO_CARD -> MCS_CARD_DETECTED\n" );
                    mount_state = MCS_CARD_DETECTED;
                } else {
                    _D2( "MCS_NO_CARD -> Exit: MC_NOT_MOUNTED\n" );
                    return MC_NOT_MOUNTED;
                }
                break;

            case MCS_CARD_DETECTED:
                _D2( "MCS_CARD_DETECTED:\n" );
                /* Make sure the power is stable. */
                vTaskDelay( TASK_DELAY_MS(250) );
                _D2( "MCS_CARD_DETECTED -> MCS_POWERED_ON\n" );
                mount_state = MCS_POWERED_ON;
                break;

            case MCS_POWERED_ON:
            {
                int i;
                _D2( "MCS_POWERED_ON:\n" );
                spi_enable( MC_SPI );

                /* Send 74+ clock cycles. */
                for( i = 0; i < 10; i++ ) {
                    io_send_dummy();
                }

                _D2( "MCS_POWERED_ON -> MCS_ASSERT_SPI_MODE\n" );
                mount_state = MCS_ASSERT_SPI_MODE;
                break;
            }

            case MCS_ASSERT_SPI_MODE:
                _D2( "MCS_ASSERT_SPI_MODE:\n" );
                /* Initialize the card into idle state. */
                if( MC_RETURN_OK == __send_card_to_idle_state() ) {
                    _D2( "MCS_ASSERT_SPI_MODE -> MCS_INTERFACE_CONDITION_CHECK\n" );
                    mount_state = MCS_INTERFACE_CONDITION_CHECK;
                } else {
                    _D2( "MCS_ASSERT_SPI_MODE -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_INTERFACE_CONDITION_CHECK:
            {
                bool legal_command;

                _D2( "MCS_INTERFACE_CONDITION_CHECK:\n" );
                /* See if this card is an SD 2.00 compliant card. */
                if( MC_RETURN_OK == mc_send_if_cond(MC_VOLTAGE, &legal_command) ) {
                    if( true == legal_command ) {
                        _D2( "MCS_INTERFACE_CONDITION_CHECK -> MCS_INTERFACE_CONDITION_SUCCESS\n" );
                        mount_state = MCS_INTERFACE_CONDITION_SUCCESS;
                    } else {
                        _D2( "MCS_INTERFACE_CONDITION_CHECK -> MCS_INTERFACE_CONDITION_FAILURE\n" );
                        mount_state = MCS_INTERFACE_CONDITION_FAILURE;
                    }
                } else {
                    _D2( "MCS_INTERFACE_CONDITION_CHECK -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;
            }

            case MCS_INTERFACE_CONDITION_SUCCESS:
                _D2( "MCS_INTERFACE_CONDITION_SUCCESS:\n" );
                /* See if this card has a compatible voltage range. */
                if( MC_RETURN_OK == __is_card_voltage_compat() ) {
                    _D2( "MCS_INTERFACE_CONDITION_SUCCESS -> MCS_SD20_COMPATIBLE_VOLTAGE\n" );
                    mount_state = MCS_SD20_COMPATIBLE_VOLTAGE;
                } else {
                    _D2( "MCS_INTERFACE_CONDITION_SUCCESS -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_INTERFACE_CONDITION_FAILURE:
                _D2( "MCS_INTERFACE_CONDITION_FAILURE:\n" );
                /* See if this card has a compatible voltage range. */
                if( MC_RETURN_OK == __is_card_voltage_compat() ) {
                    _D2( "MCS_INTERFACE_CONDITION_FAILURE -> MCS_SD10_COMPATIBLE_VOLTAGE\n" );
                    mount_state = MCS_SD10_COMPATIBLE_VOLTAGE;
                } else {
                    _D2( "MCS_INTERFACE_CONDITION_FAILURE -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_SD10_COMPATIBLE_VOLTAGE:
            {
                bool ready;

                _D2( "MCS_SD10_COMPATIBLE_VOLTAGE:\n" );
                do {
                    if( MC_RETURN_OK != mc_send_sd_op_cond(false, &ready) ) {
                        _D2( "MCS_SD10_COMPATIBLE_VOLTAGE -> MCS_MMC_CARD\n" );
                        mount_state = MCS_MMC_CARD;
                        break;
                    }
                } while( false == ready );
                _D2( "MCS_SD10_COMPATIBLE_VOLTAGE -> MCS_SD10_CARD_READY\n" );
                mount_state = MCS_SD10_CARD_READY;
                break;
            }

            case MCS_SD20_COMPATIBLE_VOLTAGE:
            {
                bool ready;

                _D2( "MCS_SD20_COMPATIBLE_VOLTAGE:\n" );
                do {
                    if( MC_RETURN_OK != mc_send_sd_op_cond(true, &ready) ) {
                        _D2( "MCS_SD20_COMPATIBLE_VOLTAGE -> MCS_CARD_UNUSABLE\n" );
                        mount_state = MCS_CARD_UNUSABLE;
                        break;
                    }
                } while( false == ready );
                _D2( "MCS_SD20_COMPATIBLE_VOLTAGE -> MCS_SD20_CARD_READY\n" );
                mount_state = MCS_SD20_CARD_READY;
                break;
            }

            case MCS_SD10_CARD_READY:
                _D2( "MCS_SD10_CARD_READY:\n" );
                mc_type = MCT_SD;
                _D2( "MCS_SD10_CARD_READY -> MCS_DETERMINE_METRICS\n" );
                mount_state = MCS_DETERMINE_METRICS;
                break;

            case MCS_SD20_CARD_READY:
            {
                mc_ocr_t ocr;

                _D2( "MCS_SD20_CARD_READY:\n" );
                if( MC_RETURN_OK == mc_read_ocr(&ocr) ) {
                    if( true == ocr.card_capacity_status ) {
                        mc_type = MCT_SDHC;
                    } else {
                        mc_type = MCT_SD_20;
                    }
                    _D2( "MCS_SD20_CARD_READY -> MCS_DETERMINE_METRICS\n" );
                    mount_state = MCS_DETERMINE_METRICS;
                } else {
                    _D2( "MCS_SD20_CARD_READY -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;
            }

            case MCS_MMC_CARD:
                _D2( "MCS_MMC_CARD:\n" );
                if( MC_RETURN_OK == __send_card_to_idle_state() ) {
                    _D2( "MCS_MMC_CARD -> MCS_INTERFACE_CONDITION_CHECK\n" );
                    mount_state = MCS_INTERFACE_CONDITION_CHECK;
                } else {
                    _D2( "MCS_MMC_CARD -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_DETERMINE_METRICS:
                _D2( "MCS_DETERMINE_METRICS:\n" );
                if( MC_RETURN_OK == __determine_metrics() ) {
                    _D2( "MCS_DETERMINE_METRICS -> MCS_SET_BLOCK_SIZE\n" );
                    mount_state = MCS_SET_BLOCK_SIZE;
                } else {
                    _D2( "MCS_DETERMINE_METRICS -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_SET_BLOCK_SIZE:
                _D2( "MCS_SET_BLOCK_SIZE:\n" );
                if( MC_RETURN_OK == __set_block_size() ) {
                    _D2( "MCS_SET_BLOCK_SIZE -> MCS_CARD_READY\n" );
                    mount_state = MCS_CARD_READY;
                } else {
                    _D2( "MCS_SET_BLOCK_SIZE -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_CARD_READY:
                _D2( "MCS_CARD_READY:\n" );

                _D1( "Mounted Card type: " );
                switch( mc_type ) {
                    case MCT_UNKNOWN:   _D1( "Unknown" ); break;
                    case MCT_MMC:       _D1( "MMC"     ); break;
                    case MCT_SD:        _D1( "SD"      ); break;
                    case MCT_SD_20:     _D1( "SD 2.0"  ); break;
                    case MCT_SDHC:      _D1( "SDHC"    ); break;
                }

                _D1( " Size: %llu\n", mc_size );

                _D2( "MCS_CARD_READY -> Exit: MC_RETURN_OK\n" );
                return MC_RETURN_OK;
                
            case MCS_CARD_UNUSABLE:
                _D2( "MCS_CARD_UNUSABLE: (%d)\n", retries );
                /* I don't like retrying, but for now it works. */
                if( 0 < retries-- ) {
                    _D2( "MCS_CARD_UNUSABLE -> MCS_NO_CARD\n" );
                    mount_state = MCS_NO_CARD;
                } else {
                    _D2( "MCS_CARD_UNUSABLE -> Exit: MC_NOT_MOUNTED\n" );
                    return MC_NOT_MOUNTED;
                }
                break;
        }
    }
}


/**
 *  Used to call all the registered callbacks.
 *
 *  @param status the status to send all the callbacks
 */
static void __call_all( const mc_card_status_t status )
{
    int32_t i;

    for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
        if( NULL != __callback_fns[i] ) {
            (*__callback_fns[i])( status );
        }
    }
}

static bool __card_present( void )
{
#if (0 != MC_CD_ACTIVE_LOW)
    return (0 == gpio_read_pin(MC_CD_PIN));
#else
    return (0 != gpio_read_pin(MC_CD_PIN));
#endif
}
