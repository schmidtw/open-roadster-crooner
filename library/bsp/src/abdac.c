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
#include "boards/boards.h"

#if (1 == AUDIO_DAC_PRESENT)

#include <stdbool.h>
#include <stdint.h>

#include <avr32/io.h>

#include <linked-list/linked-list.h>

#include "abdac.h"
#include "pdca.h"
#include "pm.h"
#include "gpio.h"
#include "intc.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* 2000 samples, x2 for stereo */
#define SILENCE_SAMPLES     (2000 * 2)
#define ABDAC_PDCA_ISR      PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_DAC )

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static abdac_buffer_done_cb __buffer_done_cb;
static volatile ll_list_t __play_list;
static volatile bool __no_more_data;
static volatile bool __silence_in_use;
static abdac_node_t __silent_node_1;
static abdac_node_t __silent_node_2;

static volatile uint32_t __underflow_count;

static const int16_t __silence[SILENCE_SAMPLES] = {
    0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,
    0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,
    0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,
    0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,
    0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0,   0, 0
};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
__attribute__ ((__interrupt__))
void __abdac_isr( void );
__attribute__ ((__interrupt__))
void __abdac_underflow( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See abdac.h for details. */
bsp_status_t abdac_init( const bool swap_channels,
                         abdac_buffer_done_cb buffer_done )
{
    if( NULL == buffer_done ) {
        return BSP_ERROR_PARAMETER;
    }

    /* Reset the silent nodes */
    ll_init_node( &__silent_node_1.node, &__silent_node_1 );
    __silent_node_1.buffer = (uint8_t*) __silence;
    __silent_node_1.size = sizeof(__silence);

    ll_init_node( &__silent_node_2.node, &__silent_node_2 );
    __silent_node_2.buffer = (uint8_t*) __silence;
    __silent_node_2.size = sizeof(__silence);

    __silence_in_use = false;

    /* Reset all settings. */
    AVR32_ABDAC.cr = 0;
    AVR32_ABDAC.idr = AVR32_ABDAC_IDR_TX_READY_MASK |
                      AVR32_ABDAC_IDR_UNDERRUN_MASK;

    pdca_channel_init( PDCA_CHANNEL_ID_DAC, AVR32_PDCA_PID_ABDAC_TX, 32 );

    intc_register_isr( &__abdac_isr, ABDAC_PDCA_ISR, ISR_LEVEL__2 );
    intc_register_isr( &__abdac_underflow, AUDIO_DAC_ISR, ISR_LEVEL__2 );

    /* Setup the mute functionality */
    gpio_set_options( AUDIO_DAC_MUTE_PIN,
                      GPIO_DIRECTION__OUTPUT,
                      GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE,
                      GPIO_INTERRUPT__NONE, 0 );

    /* Init the DAC */
    gpio_enable_module_pin( AUDIO_DAC_R_PIN, AUDIO_DAC_R_FUNCTION );
    gpio_enable_module_pin( AUDIO_DAC_L_PIN, AUDIO_DAC_L_FUNCTION );

    if( true == swap_channels ) {
        AVR32_ABDAC.CR.swap = 1;
    }

    /* Setup local variables */
    ll_init_list( &__play_list );

    __buffer_done_cb = buffer_done;

    __no_more_data = true;

    return BSP_RETURN_OK;
}

/* See abdac.h for details. */
bsp_status_t abdac_set_sample_rate( const abdac_sample_rate_t rate )
{
#if ((BOARD == CROONER_1_0) || (BOARD == CROONER_2_0))
    switch( rate ) {
        case ABDAC_SAMPLE_RATE__44100:
        case ABDAC_SAMPLE_RATE__22050:
        case ABDAC_SAMPLE_RATE__11025:
            pm_enable_pll( PM__PLL1, PM__OSC0, 14, 1, false, 16 );
            if( ABDAC_SAMPLE_RATE__44100 == rate ) {
                AVR32_PM.GCCTRL[5].div = 7;
            } else if( ABDAC_SAMPLE_RATE__22050 == rate ) {
                AVR32_PM.GCCTRL[5].div = 15;
            } else if( ABDAC_SAMPLE_RATE__11025 == rate ) {
                AVR32_PM.GCCTRL[5].div = 31;
            }
            AVR32_PM.GCCTRL[5].diven = 1;
            AVR32_PM.GCCTRL[5].pllsel = 1;
            AVR32_PM.GCCTRL[5].oscsel = 1;
            AVR32_PM.GCCTRL[5].cen = 1;
            break;


        case ABDAC_SAMPLE_RATE__48000:
        case ABDAC_SAMPLE_RATE__32000:
        case ABDAC_SAMPLE_RATE__24000:
        case ABDAC_SAMPLE_RATE__16000:
        case ABDAC_SAMPLE_RATE__12000:
        case ABDAC_SAMPLE_RATE__8000:
        default:
            return BSP_ERROR_PARAMETER;
    }

    return BSP_RETURN_OK;
#else
    return BSP_ERROR_PARAMETER;
#endif
}

/* See abdac.h for details. */
bsp_status_t abdac_queue_data( abdac_node_t *node, const bool last )
{
    volatile avr32_pdca_channel_t *abdac;
    bool isrs_enabled;
    bsp_status_t status;

    if( (NULL == node) ||
        /* Buffer size must hold 2 * (2 byte samples) evenly */
        (0 == node->size) || (0 != (0x03 & node->size)) ||
        (NULL == node->buffer) )
    {
        return BSP_ERROR_PARAMETER;
    }

    status = pdca_queue_buffer( PDCA_CHANNEL_ID_DAC, node->buffer, node->size );

    __no_more_data = last;

    if( BSP_RETURN_OK != status ) {
        /* The pdca queue buffer is only 2, & it is full - that's ok. */
        ll_append( &__play_list, &node->node );
        return BSP_RETURN_OK;
    }

    abdac = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_DAC];

    isrs_enabled = interrupts_save_and_disable();
    if( NULL == __play_list.head ) {
        /* Enable the 'transfer complete' ISR */
        abdac->ier = AVR32_PDCA_TRC_MASK;
    } else {
        /* Enable the 'refill counter zero' ISR */
        abdac->ier = AVR32_PDCA_RCZ_MASK;
    }
    /* Clear the ISR flags. */
    abdac->isr;

    ll_append( &__play_list, &node->node );
    interrupts_restore( isrs_enabled );

    return BSP_RETURN_OK;
}

/* See abdac.h for details. */
bsp_status_t abdac_queue_silence( void )
{
    if( false == __silence_in_use ) {
        abdac_queue_data( &__silent_node_1, false );
        abdac_queue_data( &__silent_node_2, false );
        __silence_in_use = true;

        return BSP_RETURN_OK;
    }

    return BSP_ABDAC_SILENCE_ALREADY_IN_USE;
}

/* See abdac.h for details. */
bsp_status_t abdac_start( void )
{
    AVR32_ABDAC.CR.en = 1;

    /* Enable the underrun counter ISR */
    AVR32_ABDAC.IER.underrun = 1;
    __underflow_count = 0;

    pdca_enable( PDCA_CHANNEL_ID_DAC );

    return BSP_RETURN_OK;
}

/* See abdac.h for details. */
bsp_status_t abdac_pause( void )
{
    gpio_clr_pin( AUDIO_DAC_MUTE_PIN );

    AVR32_ABDAC.CR.en = 0;

    return BSP_RETURN_OK;
}

/* See abdac.h for details. */
bsp_status_t abdac_resume( void )
{
    AVR32_ABDAC.CR.en = 1;

    pdca_enable( PDCA_CHANNEL_ID_DAC );

    gpio_set_pin( AUDIO_DAC_MUTE_PIN );

    return BSP_RETURN_OK;
}

/* See abdac.h for details. */
bsp_status_t abdac_stop( void )
{
    volatile avr32_pdca_channel_t *abdac;
    ll_node_t *node;
    bool isrs_enabled;

    abdac = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_DAC];

    /* Disable the 'transfer complete' and 'refill counter zero' ISRs */
    abdac->idr = AVR32_PDCA_TRC_MASK | AVR32_PDCA_RCZ_MASK;

    /* Disable the underrun counter ISR */
    AVR32_ABDAC.IDR.underrun = 1;

    abdac_pause();

    /* Disable 'abdac' pdca channel */
    abdac->cr = AVR32_PDCA_CR_TDIS_MASK;

    isrs_enabled = interrupts_save_and_disable();
    while( NULL != (node = ll_remove_head(&__play_list)) ) {
        __buffer_done_cb( (abdac_node_t*) node->data, (NULL == node->next) );
    }
    interrupts_restore( isrs_enabled );

    __no_more_data = false;

    return BSP_RETURN_OK;
}

/* See abdac.h for details. */
uint32_t abdac_get_underflow( void )
{
    return __underflow_count;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  This is called when a buffer is completed & does the needed ISR cleanup
 *  and buffer management.
 */
__attribute__ ((__interrupt__))
void __abdac_isr( void )
{
    volatile avr32_pdca_channel_t *abdac;
    ll_node_t *complete_buffer;
    bool last_buffer;

    last_buffer = false;

    abdac = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_DAC];

    complete_buffer = ll_remove_head( &__play_list );

    if( &__silent_node_1.node == complete_buffer ) {
        gpio_set_pin( AUDIO_DAC_MUTE_PIN );
    }
    if( &__silent_node_2.node == complete_buffer ) {
        __silence_in_use = false;
    }

    /* If we don't disable the interrupts like this, the interrupts continue
     * to interrupt & the system is completely un-usable. */
    if( 1 == abdac->isr ) {
        if( NULL == __play_list.head ) {
            /* Disable the 'refill counter zero' ISRs */
            abdac->idr = AVR32_PDCA_RCZ_MASK;
        } else {
            ll_node_t *node;

            /* There is 1 node, are there 2?  If there are, queue
             * that into the pdca buffer queue. */
            node = __play_list.head->next;
            if( NULL != node ) {
                abdac_node_t *abdac_node;

                abdac_node = (abdac_node_t*) node->data;
                /* Super-optimize pdca_queue_buffer() */
                abdac->marr = (unsigned long) abdac_node->buffer;
                abdac->TCRR.tcrv = (abdac_node->size >> 2);
            } else {
                /* Disable the 'refill counter zero' ISRs */
                abdac->idr = AVR32_PDCA_RCZ_MASK;
            }
        }
    } else {
        /* Disable the 'transfer complete' ISRs */
        abdac->idr = AVR32_PDCA_TRC_MASK;

        if( (NULL == __play_list.head) && (true == __no_more_data) ) {
            /* We're done. */
            last_buffer = true;
            abdac_stop();
        }
    }

    /* Clear the ISR flags. */
    abdac->isr;

    if( (NULL != complete_buffer) &&
        (&__silent_node_1.node != complete_buffer) &&
        (&__silent_node_2.node != complete_buffer) )
    {
        __buffer_done_cb( (abdac_node_t*) complete_buffer->data, last_buffer );
    }
}

/**
 *  This is called when there is an audio underflow.
 */
__attribute__ ((__interrupt__))
void __abdac_underflow( void )
{
    AVR32_ABDAC.isr;

    AVR32_ABDAC.ICR.underrun = 1;

    __underflow_count++;
}

#endif
