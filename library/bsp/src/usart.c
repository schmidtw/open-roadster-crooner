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

#include <stdbool.h>
#include <stdint.h>

#include <avr32/io.h>

#include "intc.h"
#include "pm.h"
#include "usart.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

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
/* See usart.h for details. */
bsp_status_t usart_reset( volatile avr32_usart_t *usart )
{
    bool global_interrupt_enabled;

    if( NULL == usart ) {
        return BSP_ERROR_PARAMETER;
    }

    global_interrupt_enabled = interrupts_save_and_disable();

    usart->idr = 0xffffffff;
    usart->csr;

    interrupts_restore( global_interrupt_enabled );

    usart->mr   = 0;
    usart->rtor = 0;
    usart->ttgr = 0;

    usart->cr = AVR32_USART_CR_RSTRX_MASK   |
                AVR32_USART_CR_RSTTX_MASK   |
                AVR32_USART_CR_RSTSTA_MASK  |
                AVR32_USART_CR_RSTIT_MASK   |
                AVR32_USART_CR_RSTNACK_MASK |
                AVR32_USART_CR_DTRDIS_MASK  |
                AVR32_USART_CR_RTSDIS_MASK;

    return BSP_RETURN_OK;
}

/* See usart.h for details. */
bsp_status_t usart_init_rs232( volatile avr32_usart_t *usart,
                               const usart_options_t *opt )
{
    uint32_t pba_hz;
    uint32_t over;
    uint32_t cd;
    uint32_t fp;

    if( (NULL == usart) || (NULL == opt) ) {
        return BSP_ERROR_PARAMETER;
    }

    pba_hz = pm_get_frequency( PM__PBA );
    if( 0 == pba_hz ) {
        return BSP_ERROR_CLOCK_NOT_SET;
    }

    /* Calculate the USART speed parameters. */
    over = (pba_hz >= 16 * opt->baudrate) ? 16 : 8;
    cd = pba_hz / (over * opt->baudrate);
    fp = (((32 / over) * pba_hz) / opt->baudrate - (32 * cd) + 3) / 4;

    if( (cd < 1) || (65535 < cd) || (7 < fp) ) {
        return BSP_ERROR_PARAMETER;
    }

    /* We passed the parameter checking. */

    gpio_enable_module( opt->map, opt->map_size );

    usart_reset( usart );

    usart->MR.over = (16 == over) ? AVR32_USART_MR_OVER_X16 : AVR32_USART_MR_OVER_X8;

    usart->BRGR.cd = cd;
    usart->BRGR.fp = fp;

    if( USART_DATA_BITS__9 == opt->data_bits ) {
        usart->MR.mode9 = 1;
    } else {
        usart->MR.chrl = opt->data_bits;
    }

    usart->MR.par    = opt->parity;
    usart->MR.chmode = opt->mode;
    usart->MR.nbstop = opt->stop_bits;

    if( false == opt->hw_handshake ) {
        usart->MR.mode = AVR32_USART_MR_MODE_NORMAL;
    } else {
        usart->MR.mode = AVR32_USART_MR_MODE_HARDWARE;
    }

    /* Enable the communication. */
    if( true == opt->tx_only ) {
        usart->cr = AVR32_USART_CR_TXEN_MASK;
    } else {
        usart->cr = AVR32_USART_CR_RXEN_MASK | AVR32_USART_CR_TXEN_MASK;
    }

    return BSP_RETURN_OK;
}

/* See usart.h for details. */
__attribute__ ((__always_inline__))
__inline__ bool usart_tx_ready( volatile avr32_usart_t *usart )
{
    if( NULL != usart ) {
        return (0 != (AVR32_USART_CSR_TXRDY_MASK & usart->csr));
    }

    return false;
}

/* See usart.h for details. */
bsp_status_t usart_write_char( volatile avr32_usart_t *usart, int c )
{
    if( true == usart_tx_ready(usart) ) {
        usart->THR.txchr = c;
        return BSP_RETURN_OK;
    }

    return BSP_USART_TX_BUSY;
}

/* See usart.h for details. */
bsp_status_t usart_read_char( volatile avr32_usart_t *usart, int *c )
{
    if( usart->csr & (AVR32_USART_CSR_OVRE_MASK  |
                      AVR32_USART_CSR_FRAME_MASK |
                      AVR32_USART_CSR_PARE_MASK) )
    {
        return BSP_USART_RX_ERROR;
    }

    if( 0 != usart->CSR.rxrdy ) {
        *c = usart->RHR.rxchr;
        return BSP_RETURN_OK;
    }

    return BSP_USART_RX_EMPTY;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
