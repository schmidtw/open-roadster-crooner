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
typedef struct {
    usart_new_char_handler_t new_char_fn;
    usart_cts_change_handler_t cts_fn;
    bool periodic;
} usart_callbacks_t;

/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static usart_callbacks_t __callbacks[AVR32_USART_NUM];

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
#if (0 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr0( void );
#endif
#if (1 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr1( void );
#endif
#if (2 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr2( void );
#endif
#if (3 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr3( void );
#endif
static inline void __usart_isr( const uint32_t index,
                                volatile avr32_usart_t* usart );

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
    struct usart_map {
        const avr32_usart_t *usart;
        const intc_handler_t isr_fn;
        const intc_isr_t isr;
    };

    const struct usart_map map[] = {
#if (0 < AVR32_USART_NUM)
        { .usart = (avr32_usart_t*) AVR32_USART0_ADDRESS, .isr_fn = __usart_isr0, .isr = ISR__USART_0 }
#endif
#if (1 < AVR32_USART_NUM)
       ,{ .usart = (avr32_usart_t*) AVR32_USART1_ADDRESS, .isr_fn = __usart_isr1, .isr = ISR__USART_1 }
#endif
#if (2 < AVR32_USART_NUM)
       ,{ .usart = (avr32_usart_t*) AVR32_USART2_ADDRESS, .isr_fn = __usart_isr2, .isr = ISR__USART_2 }
#endif
#if (3 < AVR32_USART_NUM)
       ,{ .usart = (avr32_usart_t*) AVR32_USART3_ADDRESS, .isr_fn = __usart_isr3, .isr = ISR__USART_3 }
#endif
    };

    uint32_t pba_hz;
    uint32_t over;
    uint32_t cd;
    uint32_t fp;
    uint32_t index;
    uint32_t timeout_bits;
    uint32_t cr;
    bool register_isr;

    if( (NULL == usart) || (NULL == opt) ) {
        return BSP_ERROR_PARAMETER;
    }

    for( index = 0;
         ((index < AVR32_USART_NUM) && (map[index].usart != usart));
         index++ )
    {
        ;
    }

    if( AVR32_USART_NUM == index ) {
        return BSP_ERROR_PARAMETER;
    }

    if( (NULL == opt->new_char_fn) &&
        ((USART_DIRECTION__IN == opt->dir) ||
         (USART_DIRECTION__BOTH == opt->dir)) )
    {
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

    /* Calculate the USART timeout (in bit time) value. */
    timeout_bits = 0;
    if( 0 < opt->timeout_us ) {
        timeout_bits = (opt->timeout_us * opt->baudrate) / 1000000;
    }

    if( (cd < 1) || (UINT16_MAX < cd) || (7 < fp) ||
        (UINT16_MAX < timeout_bits) )
    {
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

    /* This is what we'll enable for the control register. */
    cr = 0;

    __callbacks[index].new_char_fn = NULL;
    __callbacks[index].cts_fn = NULL;
    __callbacks[index].periodic = opt->periodic;

    register_isr = false;

    /* Setup the async RX if there is a function pointer. */
    if( NULL != opt->new_char_fn ) {
        __callbacks[index].new_char_fn = opt->new_char_fn;

        usart->IER.timeout = 1;     /* Didn't receive a character in time interrupt. */
        usart->IER.rxrdy = 1;       /* Got a character interrupt. */
        if( USART_PARITY__NONE != opt->parity ) {
            usart->IER.pare = 1;    /* Parity error during reception. */
        }
        usart->RTOR.to = timeout_bits;

        if( false == opt->periodic ) {
            cr |= AVR32_USART_CR_STTTO_MASK;
        }
        register_isr = true;
    }

    if( NULL != opt->cts_fn ) {
        __callbacks[index].cts_fn = opt->cts_fn;
        usart->IER.ctsic = 1;   /* enable CTS change interrupt. */
        register_isr = true;
    }

    if( true == register_isr ) {
        intc_register_isr( map[index].isr_fn, map[index].isr, ISR_LEVEL__2 );
    }

    /* Enable the communication. */
    switch( opt->dir ) {
        case USART_DIRECTION__IN:
            cr |= AVR32_USART_CR_RXEN_MASK;
            break;
        case USART_DIRECTION__OUT:
            cr |= AVR32_USART_CR_TXEN_MASK;
            break;
        case USART_DIRECTION__BOTH:
            cr |= AVR32_USART_CR_RXEN_MASK | AVR32_USART_CR_TXEN_MASK;
            break;
    }
    usart->cr = cr;

    return BSP_RETURN_OK;
}

/* See usart.h for details. */
__attribute__ ((__always_inline__))
inline bool usart_tx_ready( volatile avr32_usart_t *usart )
{
    if( NULL != usart ) {
        return (0 != (AVR32_USART_CSR_TXRDY_MASK & usart->csr));
    }

    return false;
}

/* See usart.h for details. */
__attribute__ ((__always_inline__))
inline bool usart_is_cts( volatile avr32_usart_t *usart )
{
    if( NULL != usart ) {
        return (0 == usart->CSR.cts);
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

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
#if (0 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr0( void )
{
    __usart_isr( 0, &AVR32_USART0 );
}
#endif
#if (1 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr1( void )
{
    __usart_isr( 1, &AVR32_USART1 );
}
#endif
#if (2 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr2( void )
{
    __usart_isr( 2, &AVR32_USART2 );
}
#endif
#if (3 < AVR32_USART_NUM)
__attribute__ ((__interrupt__))
static void __usart_isr3( void )
{
    __usart_isr( 3, &AVR32_USART3 );
}
#endif

static inline void __usart_isr( const uint32_t index,
                                volatile avr32_usart_t* usart )
{
    uint32_t csr_value;
    avr32_usart_csr_t *csr;

    csr_value = usart->csr;
    csr_value &= usart->imr;    /* only look at the bits we care about. */

    csr = (avr32_usart_csr_t*) &csr_value;

    if( csr->timeout ) {        /* Timed out waiting for the next character. */

        /* We need to reset the timer setting we wait until the
         * next character is received before starting the timer
         * again. */
        if( false == __callbacks[index].periodic ) {
            usart->cr = AVR32_USART_CR_STTTO_MASK;
        }

        (*__callbacks[index].new_char_fn)( -1 );

    } else if( csr->pare ) {    /* parity error */

        /* clear out any garbled data that may be present. */
        usart->RHR.rxchr;

        usart->CR.rststa = 1;
        usart->cr;

        (*__callbacks[index].new_char_fn)( -2 );

    } else if( csr->rxrdy ) {   /* normal character */
        int c;

        c = usart->RHR.rxchr;

        (*__callbacks[index].new_char_fn)( c );
    }

    /* This can happen at the same time, so it should be separate. */
    if( csr->cts ) {
        (*__callbacks[index].cts_fn)( (0 == csr->cts) ? true : false );
    }
}
