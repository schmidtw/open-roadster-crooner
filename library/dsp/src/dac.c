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
#include <stdio.h>

#include <avr32/io.h>

#include <bsp/boards/boards.h>
#include <bsp/gpio.h>
#include <bsp/intc.h>
#include <bsp/pm.h>
#include <bsp/pdca.h>

#include "dac.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DAC_PDCA_ISR    PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_DAC )

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    const uint32_t bitrate;
    const uint32_t mult;
    const uint32_t div;
    const bool divide_by_two;
    const uint32_t gcctrl_div;
} bitrate_map_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */
static const bitrate_map_t bitrate_map[] = {
#if ((BOARD == CROONER_1_0) || (BOARD == CROONER_2_0))
    { .bitrate = 44100, .mult = 14, .div = 1, .divide_by_two = false, .gcctrl_div = 7  },
    { .bitrate = 22050, .mult = 14, .div = 1, .divide_by_two = false, .gcctrl_div = 15 },
    { .bitrate = 11025, .mult = 14, .div = 1, .divide_by_two = false, .gcctrl_div = 31 },

    { .bitrate = 48000, .mult = 7,  .div = 1, .divide_by_two = false, .gcctrl_div = 3  },
    { .bitrate = 32000, .mult = 14, .div = 1, .divide_by_two = false, .gcctrl_div = 10 },
    { .bitrate = 24000, .mult = 7,  .div = 1, .divide_by_two = false, .gcctrl_div = 7  },
    { .bitrate = 16000, .mult = 14, .div = 1, .divide_by_two = false, .gcctrl_div = 21 },
    { .bitrate = 12000, .mult = 7,  .div = 1, .divide_by_two = false, .gcctrl_div = 9  },
    { .bitrate =  8000, .mult = 14, .div = 1, .divide_by_two = false, .gcctrl_div = 43 },
#endif
    { .bitrate =     0, .mult = 0,  .div = 0, .divide_by_two = false, .gcctrl_div = 0  }
};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See dac.h for details. */
void dac_init( void (*complete_isr)(void),
               const bool swap_channels )
{
    /* Reset all settings. */
    AVR32_ABDAC.cr = 0;
    AVR32_ABDAC.idr = AVR32_ABDAC_IDR_TX_READY_MASK |
                      AVR32_ABDAC_IDR_UNDERRUN_MASK;

    pdca_channel_init( PDCA_CHANNEL_ID_DAC, AVR32_PDCA_PID_ABDAC_TX, 32 );

    /* Enable the transfer ISR settings. */
    intc_register_isr( complete_isr, DAC_PDCA_ISR, ISR_LEVEL__2 );

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
}

/* See dac.h for details. */
void dac_start( void )
{
    pdca_isr_enable( PDCA_CHANNEL_ID_DAC, PDCA_ISR__TRANSFER_COMPLETE );
    pdca_isr_enable( PDCA_CHANNEL_ID_DAC, PDCA_ISR__RELOAD_COUNTER_ZERO );

    pdca_enable( PDCA_CHANNEL_ID_DAC );

    AVR32_ABDAC.CR.en = 1;

    gpio_set_pin( AUDIO_DAC_MUTE_PIN );
}

/* See dac.h for details. */
void dac_pause( void )
{
    gpio_clr_pin( AUDIO_DAC_MUTE_PIN );

    AVR32_ABDAC.CR.en = 0;
}

/* See dac.h for details. */
void dac_stop( void )
{
    dac_pause();

    /* Clear out any pending buffers so we don't get
     * audio blips when we re-enable the audio. */
    pdca_disable( PDCA_CHANNEL_ID_DAC );
}

/* See dac.h for details. */
dsp_status_t dac_set_sample_rate( const uint32_t rate )
{
    int i;

    for( i = 0; i < sizeof(bitrate_map)/sizeof(bitrate_map_t) - 1; i++ ) {
        if( rate == bitrate_map[i].bitrate ) {
            pm_enable_pll( PM__PLL1, PM__OSC0, bitrate_map[i].mult,
                           bitrate_map[i].div, bitrate_map[i].divide_by_two,
                           16 );
            AVR32_PM.GCCTRL[5].div = bitrate_map[i].gcctrl_div;
            AVR32_PM.GCCTRL[5].diven = 1;
            AVR32_PM.GCCTRL[5].pllsel = 1;
            AVR32_PM.GCCTRL[5].oscsel = 1;
            AVR32_PM.GCCTRL[5].cen = 1;

            return DSP_RETURN_OK;
        }
    }

    return DSP_UNSUPPORTED_BITRATE;
}

/* See dac.h for details. */
bool dac_is_supported_bitrate( const uint32_t rate )
{
    int i;

    for( i = 0; i < sizeof(bitrate_map)/sizeof(bitrate_map_t) - 1; i++ ) {
        if( rate == bitrate_map[i].bitrate ) {
            return true;
        }
    }

    return false;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
