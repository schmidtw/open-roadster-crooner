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

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <bsp/boards/boards.h>
#include <bsp/pdca.h>
#include <freertos/queue.h>

#include "crc.h"
#include "io.h"
#include "memcard.h"
#include "timing-parameters.h"
#include "memcard-constants.h"
#include "memcard-private.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MC_RX_ISR       PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_RX )
#define MC_TX_ISR       PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_TX )

#define MIN( a, b )     ((a) < (b)) ? (a) : (b)

#define _D1(...)
#define _D2(...)

#ifdef BLOCK_DEBUG
#if (0 < BLOCK_DEBUG)
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif
#if (1 < BLOCK_DEBUG)
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    BRS_COMMAND_SENT,
    BRS_LOOKING_FOR_BLOCK,
    BRS_BLOCK_START_FOUND,
    BRS_SUCCESS,
    BRS_TIMEOUT
} block_request_state_t;

typedef struct {
    uint8_t command[MC_COMMAND_BUFFER_SIZE];
    uint8_t *data;
    uint8_t buffer[MC_BLOCK_BUFFER_SIZE];
    uint32_t length;
    uint8_t crc[2];
    uint32_t crc_length;
    block_request_state_t state;
    uint32_t nac;
} block_message_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xQueueHandle __idle;
static xQueueHandle __pending;
static xQueueHandle __complete;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __block_isr_send_command( const uint8_t *send,
                                      uint8_t *receive,
                                      const size_t length,
                                      const bool select );
static uint32_t __block_isr_find_and_process_block_start( block_message_t *msg,
                                                          const uint8_t *start,
                                                          const size_t len );
static void __block_isr_state_machine( void );
static void __block_isr_disable_transfer( void );
__attribute__((__interrupt__))
static void __block_handler( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
mc_status_t block_init( void* (*fast_malloc_fn)(size_t) )
{
    block_message_t *msg;

    __idle = xQueueCreate( 1, sizeof(void*) );
    __pending = xQueueCreate( 1, sizeof(void*) );
    __complete = xQueueCreate( 1, sizeof(void*) );

    msg = (block_message_t*) (*fast_malloc_fn)( sizeof(block_message_t) );

    pdca_channel_init( PDCA_CHANNEL_ID_MC_RX, MC_PDCA_RX_PERIPHERAL_ID, 8 );
    pdca_channel_init( PDCA_CHANNEL_ID_MC_TX, MC_PDCA_TX_PERIPHERAL_ID, 8 );

    intc_register_isr( &__block_handler, MC_RX_ISR, ISR_LEVEL__1 );

    xQueueSendToBack( __idle, &msg, portMAX_DELAY );

    return MC_RETURN_OK;
}

/* See block.h for details. */
mc_status_t block_read( const uint32_t lba, uint8_t *buffer )
{
    mc_card_status_t card_status;
    mc_status_t status;
    uint32_t address = lba;
    block_message_t *msg;
    int32_t i;

    card_status = mc_get_status();

    if( NULL == buffer ) {
        return MC_ERROR_PARAMETER;
    }

    if( (MC_CARD__MOUNTING != card_status) &&
        (MC_CARD__MOUNTED != card_status) )
    {
        return MC_ERROR_MODE;
    }

    if( MCT_SDHC != mc_get_type() ) {
        address <<= 9;
    }

    xQueueReceive( __idle, &msg, portMAX_DELAY );

    /* Fill with 0xff to start with. */
    memset( &msg->command, 0xff, MC_COMMAND_BUFFER_SIZE );

    /* Fill MC_Ncs bytes worth of data with 0xff.
     * This is needed for the timing. */
    i = MC_Ncs;

    /* Calculate the command sequence */
    msg->command[i++] = 0x40 | (0x3f & MC_READ_SINGLE_BLOCK);
    msg->command[i++] = address >> 24;
    msg->command[i++] = 0xff & (address >> 16);
    msg->command[i++] = 0xff & (address >> 8);
    msg->command[i++] = 0xff & address;
    msg->command[i++] = (crc7(&msg->command[MC_Ncs], 5) << 1) | 0x01;

    msg->data = buffer;
    msg->length = 0;
    msg->nac = 0;
    msg->crc_length = 0;

    msg->state = BRS_COMMAND_SENT;
    xQueueSendToBack( __pending, &msg, portMAX_DELAY );
    __block_isr_send_command( msg->command, msg->command,
                              MC_COMMAND_BUFFER_SIZE, true );

    _D1( "Reading: 0x%08x\n", lba );
    xQueueReceive( __complete, &msg, portMAX_DELAY );
    status = (BRS_SUCCESS == msg->state) ? MC_RETURN_OK : MC_ERROR_TIMEOUT;

    status = MC_ERROR_TIMEOUT;
    if( BRS_SUCCESS == msg->state ) {
        uint16_t crc, calc_crc;
        status = MC_CRC_FAILURE;
        crc = ((0xff & msg->crc[0]) << 8) | (0xff & msg->crc[1]);
        calc_crc = crc16( buffer, msg->length );
        if( crc == calc_crc ) {
            status = MC_RETURN_OK;
        }
    }
    xQueueSendToBack( __idle, &msg, portMAX_DELAY );

    _D1( "Got response: 0x%04x\n", status );
    return status;
}

/* See block.h for details. */
mc_status_t block_write( const uint32_t lba, const uint8_t *buffer )
{
    return MC_NOT_SUPPORTED;
}

/* See block.h for details. */
void block_isr_cancel( void )
{
    block_message_t *msg;
    portBASE_TYPE ignore;

    if( pdTRUE == xQueueReceiveFromISR(__pending, &msg, &ignore) ) {
        /* We're done... */
        msg->state = BRS_TIMEOUT;

        /* If we started to send something, cancel it. */
        pdca_isr_disable( PDCA_CHANNEL_ID_MC_TX, PDCA_ISR__TRANSFER_COMPLETE );
        pdca_isr_disable( PDCA_CHANNEL_ID_MC_RX, PDCA_ISR__TRANSFER_COMPLETE );

        /* This is a failure, so it's ok to not check the magic */
        xQueueSendToBackFromISR( __complete, &msg, &ignore );
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to send the command or data to/from the card.
 *
 *  @note ISR safe - no printf() calls.
 *
 *  @param send the data to send to the card
 *  @param receive the buffer to receive response data into
 *  @param length the length of the data to send/receive
 *  @param select if the card should be selected prior to communicating
 */
static void __block_isr_send_command( const uint8_t *send,
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

/**
 *  Used to process and find the start of a data block.
 *
 *  @note ISR safe - no printf() calls.
 *
 *  @param msg the message to process
 *  @param start the start of the block to search
 *  @param len the length of the buffer to search
 *
 *  @return the number of bytes left after the block start
 */
static uint32_t __block_isr_find_and_process_block_start( block_message_t *msg,
                                                          const uint8_t *start,
                                                          const size_t len )
{
    uint8_t *block_start;
    uint32_t left;

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

            memcpy( msg->data, block_start, MIN(512, left) );

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
        msg->state = BRS_BLOCK_START_FOUND;
    }

    return left;
}

/**
 *  Used to traverse the request through the state diagram until
 *  it is completed, or cancelled.
 *
 *  @note ISR safe - no printf() calls.
 *
 */
static void __block_isr_state_machine( void )
{
    uint32_t magic_insert_number;
    bool interrupts;
    portBASE_TYPE rv;
    portBASE_TYPE ignore;
    block_message_t *msg;

    interrupts = interrupts_save_and_disable();
    magic_insert_number = mc_get_magic_insert_number();
    rv = xQueueReceiveFromISR( __pending, &msg, &ignore );
    interrupts_restore( interrupts );

    if( pdTRUE == rv ) {
        bool send_to_pending = true;
        mc_card_status_t card_status = mc_get_status();

        if( (MC_CARD__MOUNTING != card_status) &&
            (MC_CARD__MOUNTED != card_status) )
        {
            goto timeout_failure;
        }

        if( BRS_COMMAND_SENT == msg->state ) {
            uint8_t *r1, *end;

            end = &msg->command[MC_COMMAND_BUFFER_SIZE];

            r1 = memchr( msg->command, 0, MC_COMMAND_BUFFER_SIZE );
            if( NULL == r1 ) {
                goto timeout_failure;
            }
            msg->state = BRS_LOOKING_FOR_BLOCK;

            __block_isr_find_and_process_block_start( msg, r1, (end - r1) );

            __block_isr_send_command( memory_block_dummy_data, msg->buffer,
                                      MC_BLOCK_BUFFER_SIZE, false );
        } else if( BRS_LOOKING_FOR_BLOCK == msg->state ) {
            uint32_t send;
            bool done = false;

            send = MC_BLOCK_BUFFER_SIZE;
            __block_isr_find_and_process_block_start( msg, msg->buffer,
                                                      MC_BLOCK_BUFFER_SIZE );

            if( BRS_LOOKING_FOR_BLOCK == msg->state ) {
                /* Nac is in clock cycles, there are 8 clock cycles per
                 * byte, so divide by 8 (or shift right by 3) */
                if( mc_get_Nac_read() < (msg->nac >> 3) ) {
                    goto timeout_failure;
                }
            } else {
                if( (msg->length < 512) || (msg->crc_length < 2) ) {
                    send = 512 - msg->length;
                    send += 2 - msg->crc_length;
                } else {
                    msg->state = BRS_SUCCESS;
                    done = true;
                    send_to_pending = false;
                }
            }

            if( false == done ) {
                __block_isr_send_command( memory_block_dummy_data, msg->buffer, send, false );
            }
        } else if( BRS_BLOCK_START_FOUND == msg->state ) {
            if( msg->length < 512 ) {
                uint32_t copy_length;

                copy_length = 512 - msg->length;

                /* Need to copy more of the message */
                memcpy( &msg->data[msg->length], msg->buffer, copy_length );
                msg->length += copy_length;
            
                msg->crc[0] = msg->buffer[copy_length];
                msg->crc[1] = msg->buffer[copy_length + 1];
                msg->crc_length = 2;
            }

            msg->state = BRS_SUCCESS;
            send_to_pending = false;
        }

        interrupts = interrupts_save_and_disable();
        if( magic_insert_number != mc_get_magic_insert_number() ) {
            /* The card was removed, bail. */
            interrupts_restore( interrupts );
            goto timeout_failure;
        }
        if( true == send_to_pending ) {
            xQueueSendToFrontFromISR( __pending, &msg, &ignore );
        } else {
            xQueueSendToBackFromISR( __complete, &msg, &ignore );
        }
        interrupts_restore( interrupts );
    }

    return;

timeout_failure:
    /* We're done... */
    msg->state = BRS_TIMEOUT;

    /* If we started to send something, cancel it. */
    __block_isr_disable_transfer();

    /* This is a failure, so it's ok to not check the magic */
    xQueueSendToBackFromISR( __complete, &msg, &ignore );
}

/**
 *  Used to disable the block transfer for the media card.
 *
 *  @note ISR safe - no printf() calls.
 *
 */
static void __block_isr_disable_transfer( void )
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
}

/**
 *  ISR that handles the block transfer.
 */
__attribute__((__interrupt__))
static void __block_handler( void )
{
    __block_isr_disable_transfer();
    __block_isr_state_machine();
}
