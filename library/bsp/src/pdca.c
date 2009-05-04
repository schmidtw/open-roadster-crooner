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

#include <stddef.h>
#include <stdint.h>

#include <avr32/io.h>

#include "pdca.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* For uc1a.... devices: 17 */
/* For uc1b.... devices: 12 */
#define PDCA_MAX_PIDS   17

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See pdca.h for details. */
bsp_status_t pdca_channel_init( const uint8_t channel,
                                const uint8_t dest_pid,
                                const uint8_t transfer_data_size )
{
    volatile avr32_pdca_channel_t *pdca;

    if( AVR32_PDCA_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    if( PDCA_MAX_PIDS < dest_pid ) {
        return BSP_ERROR_PARAMETER;
    }


    if( (8  != transfer_data_size) &&
        (16 != transfer_data_size) &&
        (32 != transfer_data_size) )
    {
        return BSP_ERROR_PARAMETER;
    }

    pdca = &AVR32_PDCA.channel[channel];

    /* Disable anything going on now. */
    pdca_disable( channel );

    pdca->idr = AVR32_PDCA_IDR_RCZ_MASK |
                AVR32_PDCA_IDR_TERR_MASK |
                AVR32_PDCA_IDR_TRC_MASK;

    /* Clear any pending ISRs */
    pdca->isr;

    pdca->PSR.pid = dest_pid;

    if( 8 == transfer_data_size ) {
        pdca->MR.size = AVR32_PDCA_BYTE;
    } else if( 16 == transfer_data_size ) {
        pdca->MR.size = AVR32_PDCA_HALF_WORD;
    } else {
        pdca->MR.size = AVR32_PDCA_WORD;
    }

    return BSP_RETURN_OK;
}

/* See pdca.h for details. */
bsp_status_t pdca_queue_buffer( const uint8_t channel,
                                volatile void *data,
                                const uint16_t size )
{
    volatile avr32_pdca_channel_t *pdca;
    uint16_t transfer_size;

    if( (NULL == data) || (0 == size) ||
        (AVR32_PDCA_CHANNEL_LENGTH <= channel) )
    {
        return BSP_ERROR_PARAMETER;
    }

    pdca = &AVR32_PDCA.channel[channel];

    transfer_size = size;
    if( AVR32_PDCA_WORD == pdca->MR.size ) {
        if( 0 == (0xfffc & size) ) {
            return BSP_ERROR_PARAMETER;
        }
        transfer_size >>= 2;
    } else if( AVR32_PDCA_HALF_WORD == pdca->MR.size ) {
        if( 0 == (0xfffe & size) ) {
            return BSP_ERROR_PARAMETER;
        }
        transfer_size >>= 1;
    }

    /* Either we're not transferring any data, we've only got
     * the current transfer queued & the next one is empty, or
     * both are queued & we're full. */
    if( 1 == pdca->SR.ten ) {
        /* transferring data */
        if( 0 != pdca->marr ) {
            return BSP_PDCA_QUEUE_FULL;
        } else {
            pdca->marr = (unsigned long) data;
            pdca->TCRR.tcrv = transfer_size;
        }
    } else {
        /* not transferring data */
        if( 0 == pdca->mar ) {
            pdca->mar = (unsigned long) data;
            pdca->TCR.tcv = transfer_size;
        } else if( 0 == pdca->marr ) {
            pdca->marr = (unsigned long) data;
            pdca->TCRR.tcrv = transfer_size;
        } else {
            return BSP_PDCA_QUEUE_FULL;
        }
    }
    /* Clear any pending ISRs */
    pdca->isr;

    return BSP_RETURN_OK;
}

/* See pdca.h for details. */
bsp_status_t pdca_disable( const uint8_t channel )
{
    volatile avr32_pdca_channel_t *pdca;

    if( AVR32_PDCA_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    pdca = &AVR32_PDCA.channel[channel];

    pdca->cr = AVR32_PDCA_CR_TDIS_MASK;
    pdca->mar = 0;
    pdca->tcr = 0;
    pdca->marr = 0;
    pdca->tcrr = 0;

    return BSP_RETURN_OK;
}

/* See pdca.h for details. */
bsp_status_t pdca_isr_enable( const uint8_t channel, const pdca_isr_t isr )
{
    volatile avr32_pdca_channel_t *pdca;

    if( AVR32_PDCA_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    pdca = &AVR32_PDCA.channel[channel];

    switch( isr ) {
        case PDCA_ISR__TRANSFER_ERROR:
            pdca->ier = AVR32_PDCA_TERR_MASK;
            break;

        case PDCA_ISR__TRANSFER_COMPLETE:
            pdca->ier = AVR32_PDCA_TRC_MASK;
            break;

        case PDCA_ISR__RELOAD_COUNTER_ZERO:
            pdca->ier = AVR32_PDCA_RCZ_MASK;
            break;

        default:
            return BSP_ERROR_PARAMETER;
    }

    /* Clear any pending ISRs */
    pdca->isr;

    return BSP_RETURN_OK;
}

/* See pdca.h for details. */
bsp_status_t pdca_isr_disable( const uint8_t channel, const pdca_isr_t isr )
{
    volatile avr32_pdca_channel_t *pdca;

    if( AVR32_PDCA_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    pdca = &AVR32_PDCA.channel[channel];

    switch( isr ) {
        case PDCA_ISR__TRANSFER_ERROR:
            pdca->idr = AVR32_PDCA_TERR_MASK;
            break;

        case PDCA_ISR__TRANSFER_COMPLETE:
            pdca->idr = AVR32_PDCA_TRC_MASK;
            break;

        case PDCA_ISR__RELOAD_COUNTER_ZERO:
            pdca->idr = AVR32_PDCA_RCZ_MASK;
            break;

        default:
            return BSP_ERROR_PARAMETER;
    }

    /* Clear any pending ISRs */
    pdca->isr;

    return BSP_RETURN_OK;
}

/* See pdca.h for details. */
bsp_status_t pdca_enable( const uint8_t channel )
{
    if( AVR32_PDCA_CHANNEL_LENGTH <= channel ) {
        return BSP_ERROR_PARAMETER;
    }

    AVR32_PDCA.channel[channel].cr = AVR32_PDCA_CR_ECLR_MASK |
                                     AVR32_PDCA_CR_TEN_MASK;

    return BSP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
