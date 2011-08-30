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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <freertos/os.h>

#include <bsp/boards/boards.h>
#include <bsp/intc.h>
#include <bsp/pdca.h>
#include <bsp/dac.h>

#include "dsp.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DSP_TASK_STACK_SIZE 50
#define DSP_OUT_MSG_MAX     40
#define DSP_IN_MSG_MAX      10
#define DSP_BUFFER_SIZE     441
#define DSP_SILENCE_MSG_MAX 2

#define GAIN_SCALE      8
#define OUTPUT_SCALE    13
#define DC_BIAS         (1 << (OUTPUT_SCALE - 1))
#define MIN(a,b)        ((a) < (b)) ? (a) : (b)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    int16_t samples[DSP_BUFFER_SIZE * 2];
    size_t used;
    uint32_t bitrate;
    bool silence;
} dsp_output_t;

typedef struct {
    int32_t *left;
    int32_t *right;
    size_t count;
    int32_t offset;
    uint32_t bitrate;
    int32_t gain_scale_factor;
    dsp_buffer_return_fct cb;
    void *data;
} dsp_input_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static queue_handle_t __output_idle;
static queue_handle_t __output_active;
static queue_handle_t __output_queued;
static queue_handle_t __output_idle_silence;
static dsp_output_t __output[DSP_OUT_MSG_MAX];

static queue_handle_t __input_idle;
static queue_handle_t __input_queued;
static dsp_input_t __input[DSP_IN_MSG_MAX];

static const dsp_output_t __silence[DSP_SILENCE_MSG_MAX] = {
    { .used = DSP_BUFFER_SIZE, .silence = true, .bitrate = 0 },
    { .used = DSP_BUFFER_SIZE, .silence = true, .bitrate = 0 } };

static volatile uint32_t __bitrate;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __dsp_task( void *params );
__attribute__ ((__interrupt__)) static void __dac_buffer_complete( void );
static void __process_samples( dsp_input_t *in, dsp_output_t *out );
static int32_t __convert_gain( const double gain );
static void __mono_to_stereo_out( const int32_t *in, int16_t *out,
                                  int32_t count, const int32_t gain_scale_factor );
static void __stereo_to_stereo_out( const int32_t *l, const int32_t *r,
                                    int16_t *out, int32_t count,
                                    const int32_t gain_scale_factor );
static void __queue_request( int32_t *left,
                             int32_t *right,
                             const size_t count,
                             const uint32_t bitrate,
                             const int32_t gain_scale_factor,
                             dsp_buffer_return_fct cb,
                             void *data );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See dsp.h for details */
dsp_status_t dsp_init( const uint32_t priority )
{
    bool status;
    int32_t i;

    __bitrate = 44100;

    __output_idle = NULL;
    __output_idle_silence = NULL;
    __output_active = NULL;
    __output_queued = NULL;
    __input_idle = NULL;
    __input_queued = NULL;

    __output_idle = os_queue_create( DSP_OUT_MSG_MAX, sizeof(dsp_output_t*) );
    __output_idle_silence = os_queue_create( DSP_SILENCE_MSG_MAX, sizeof(dsp_output_t*) );
    __output_active = os_queue_create( 2, sizeof(dsp_output_t*) );
    __output_queued = os_queue_create( DSP_OUT_MSG_MAX, sizeof(dsp_output_t*) );
    __input_idle = os_queue_create( DSP_IN_MSG_MAX, sizeof(dsp_input_t*) );
    __input_queued = os_queue_create( DSP_IN_MSG_MAX, sizeof(dsp_input_t*) );

    if( (NULL == __output_idle) || (NULL == __output_idle_silence) ||
        (NULL == __output_active) || (NULL == __output_queued) ||
        (NULL == __input_idle) || (NULL == __input_queued) )
    {
        goto failure;
    }

    for( i = 0; i < DSP_OUT_MSG_MAX; i++ ) {
        dsp_output_t *out = &__output[i];
        out->silence = false;
        os_queue_send_to_back( __output_idle, &out, NO_WAIT );
    }

    for( i = 0; i < DSP_IN_MSG_MAX; i++ ) {
        dsp_input_t *in = &__input[i];
        os_queue_send_to_back( __input_idle, &in, NO_WAIT );
    }

    for( i = 0; i < DSP_SILENCE_MSG_MAX; i++ ) {
        const dsp_output_t *out = &__silence[i];
        os_queue_send_to_back( __output_idle_silence, &out, NO_WAIT );
    }

    status = os_task_create( __dsp_task, "DSP ", DSP_TASK_STACK_SIZE,
                             NULL, priority, NULL );

    if( true != status ) {
        goto failure;
    }

    dac_init( &__dac_buffer_complete, false );

    return DSP_RETURN_OK;

failure:
    if( NULL == __output_idle ) {
        os_queue_delete( __output_idle );
    }
    if( NULL == __output_idle_silence ) {
        os_queue_delete( __output_idle_silence );
    }
    if( NULL == __output_active ) {
        os_queue_delete( __output_active );
    }
    if( NULL == __output_queued ) {
        os_queue_delete( __output_queued );
    }
    if( NULL == __input_idle )  {
        os_queue_delete( __input_idle );
    }
    if( NULL == __input_queued ) {
        os_queue_delete( __input_queued );
    }

    return DSP_RESOURCE_ERROR;
}

/* See dsp.h for details */
dsp_status_t dsp_control( const dsp_cmd_t cmd )
{
    return DSP_RETURN_OK;
}

/* See dsp.h for details */
int32_t dsp_determine_scale_factor( const double peak, const double gain )
{
    return __convert_gain( gain );
}

/* See dsp.h for details */
dsp_status_t dsp_queue_data( int32_t *left,
                             int32_t *right,
                             const size_t count,
                             const uint32_t bitrate,
                             const int32_t gain_scale_factor,
                             dsp_buffer_return_fct cb,
                             void *data )
{
    if( ((NULL == left) && (NULL == right)) || (0 == count) ||
        (0 == bitrate) || (gain_scale_factor < 0) || (NULL == cb) )
    {
        return DSP_PARAMETER_ERROR;
    }

    if( false == dac_is_supported_bitrate(bitrate) ) {
        return DSP_UNSUPPORTED_BITRATE;
    }

    __queue_request( left, right, count, bitrate, gain_scale_factor, cb, data );

    return DSP_RETURN_OK;
}

/* See dsp.h for details */
void dsp_data_complete( dsp_buffer_return_fct cb,
                        void *data )
{
    __queue_request( NULL, NULL, 0, 0, 0, cb, data );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __dsp_task( void *params )
{
    dsp_output_t *out;

    dac_set_sample_rate( __bitrate );

    out = NULL;

    /* Start playing. */
    dac_start();

    while( 1 ) {
        dsp_input_t *in;

        if( true == os_queue_receive( __input_queued, &in, WAIT_FOREVER) ) {

            if( 0 == in->count ) {
                /* No more data - play silence. */
                if( NULL != out ) {
                    memset( &out->samples[out->used*2], 0,
                            (sizeof(int16_t)*2*(DSP_BUFFER_SIZE - out->used)) );
                    out->used = DSP_BUFFER_SIZE;
                    in->offset = 0;
                    os_queue_send_to_back( __output_queued, &out, WAIT_FOREVER );
                    out = NULL;
                }
            } else {
                while( in->offset < in->count ) {
                    if( NULL == out ) {
                        os_queue_receive( __output_idle, &out, WAIT_FOREVER );
                        out->used = 0;
                    }

                    __process_samples( in, out );

                    if( DSP_BUFFER_SIZE == out->used ) {
                        os_queue_send_to_back( __output_queued, &out, WAIT_FOREVER );
                        out = NULL;
                    }
                }
            }

            /* We're done - send the buffer back & re-queue the message. */
            if( NULL != in->cb ) {
                (*in->cb)( in->left, in->right, in->data );
            }
            os_queue_send_to_back( __input_idle, &in, WAIT_FOREVER );
        }
    }
}

/**
 *  This is called when a buffer is completed & does the needed ISR cleanup
 *  and buffer management.
 */
__attribute__ ((__interrupt__))
static void __dac_buffer_complete( void )
{
    static bool bitrate_change = false;
    static uint32_t new_bitrate = 0;
    dsp_output_t *out;
    bool status;

    int i;

    //intc_isr_puts( "__dac_buffer_complete()\n" );

    /* Move the transferred message to the idle queue. */
    status = os_queue_receive_ISR( __output_active, &out, NULL );
    if( true == status ) {
        if( true == out->silence ) {
            os_queue_send_to_back_ISR( __output_idle_silence, &out, NULL );
        } else {
            os_queue_send_to_back_ISR( __output_idle, &out, NULL );
        }
    }

    if( false == bitrate_change ) {
        /* Look at the first message in the queue to see if the bitrate changes. */
        status = os_queue_receive_ISR( __output_queued, &out, NULL );
        if( true == status ) {
            os_queue_send_to_front_ISR( __output_queued, &out, NULL );

            if( __bitrate != out->bitrate ) {
                bitrate_change = true;
                new_bitrate = out->bitrate;
                status = os_queue_receive_ISR( __output_idle_silence, &out, NULL );
                if( true == status ) {
                    os_queue_send_to_back_ISR( __output_active, &out, NULL );
                }
            }
        }

        /* Put the next audio clip into the playback system or add
         * silence. */
        for( i = 0; i < 2; i++ ) {
            queue_handle_t current;
            if( 0 == i ) {
                current = __output_queued;
            } else {
                current = __output_idle_silence;
            }

            while( (false == os_queue_is_full_ISR(__output_active)) &&
                   (false == os_queue_is_empty_ISR(current)) )
            {
                /* Queue the next pending buffer for playback. */
                status = os_queue_receive_ISR( current, &out, NULL );
                if( true == status ) {
                    bsp_status_t queue_status;
                    /* This can't fail because one of the two buffers just
                     * became available the only other failure is parameter error. */
                    queue_status = pdca_queue_buffer( PDCA_CHANNEL_ID_DAC,
                                                      out->samples, out->used << 2 );

                    if( BSP_RETURN_OK == queue_status ) {
                        os_queue_send_to_back_ISR( __output_active, &out, NULL );
                    } else {
                        intc_isr_puts( "Bad pdca_queue_buffer() return value\n" );
                        os_queue_send_to_front_ISR( current, &out, NULL );
                    }
                }
            }
        }
    } else {
        /* If we are in the bitrate change mode, change the bitrate and
         * queue the next silent audio clip. */
        __bitrate = new_bitrate;
        dac_set_sample_rate( __bitrate );
        bitrate_change = false;
        new_bitrate = 0;

        status = os_queue_receive_ISR( __output_idle_silence, &out, NULL );
        if( true == status ) {
            os_queue_send_to_back_ISR( __output_active, &out, NULL );
        }
    }

    /* If there is a bubble in the audio, play silence */

    if( BSP_RETURN_OK != pdca_isr_clear(PDCA_CHANNEL_ID_DAC) ) {
        intc_isr_puts( "pdca_isr_clear returned BSP_ERROR_PARAMETER\n" );
    }
}


static void __process_samples( dsp_input_t *in, dsp_output_t *out )
{
    int32_t out_size;
    int32_t in_size;
    int32_t process_size;

    out_size = DSP_BUFFER_SIZE - out->used;
    in_size = in->count - in->offset;
    process_size = MIN( out_size, in_size );

    if( (NULL == in->left) || (NULL == in->right) ) {
        int32_t *mono;

        mono = (NULL == in->left) ? in->right : in->left;

        __mono_to_stereo_out( &mono[in->offset], &out->samples[out->used*2],
                              process_size, in->gain_scale_factor );
    } else {
        __stereo_to_stereo_out( &in->right[in->offset], &in->left[in->offset],
                                &out->samples[out->used*2], process_size,
                                in->gain_scale_factor );
    }

    in->offset += process_size;
    out->used += process_size;
    out->bitrate = in->bitrate;
}

/**
 *  Used to convert the double version of the gain into the integer
 *  based version.
 *
 *  @param gain the double gain to convert
 *
 *  @return the integer version of the gain
 */
static int32_t __convert_gain( const double gain )
{
    const int32_t one = 1 << GAIN_SCALE;
    double adjusted_gain;

    /* According to http://replaygain.hydrogenaudio.org/player_scale.html */
    adjusted_gain = pow( 10, (gain / 20.0) );

    return (int32_t) (adjusted_gain * ((double) one));
}

/**
 *  Used to convert mono audio input into 16 bit audio output,
 *  including applying the desired gain.
 *
 *  @param in the input buffer of samples
 *  @param out the output buffer of converted samples
 *  @param count the number of samples to convert
 *  @param gain_scale_factor the gain multiplier
 */
static void __mono_to_stereo_out( const int32_t *in, int16_t *out,
                                  int32_t count, const int32_t gain_scale_factor )
{
    while( 0 < count ) {
        register int32_t data;

        /* Apply gain */
        data = (((int64_t) *in++) * (int64_t) gain_scale_factor) >> GAIN_SCALE;

        /* Convert from 32 bit to 16 bit */
        if( 0 < data ) {
            data += DC_BIAS;
            data >>= OUTPUT_SCALE;
            /* Clip if needed. */
            if( INT16_MAX < data ) {
                data = INT16_MAX;
            }
        } else {
            data -= DC_BIAS;
            data >>= OUTPUT_SCALE;
            /* Clip if needed. */
            if( data < INT16_MIN ) {
                data = INT16_MIN;
            }
        }

        *out++ = (int16_t) data;
        *out++ = (int16_t) data;

        count--;
    }
}

/**
 *  Used to convert stereo audio input into 16 bit audio output,
 *  including applying the desired gain.
 *
 *  @param l the left input buffer of samples
 *  @param r the right input buffer of samples
 *  @param out the output buffer of converted samples
 *  @param count the number of samples to convert
 *  @param gain_scale_factor the gain multiplier
 */
static void __stereo_to_stereo_out( const int32_t *l, const int32_t *r,
                                    int16_t *out, int32_t count,
                                    const int32_t gain_scale_factor )
{
    while( 0 < count ) {
        register int32_t data;

        /* Left sample ---------------------- */

        /* Apply gain */
        data = (((int64_t) *l++) * (int64_t) gain_scale_factor) >> GAIN_SCALE;

        /* Convert from 32 bit to 16 bit */
        if( 0 < data ) {
            data += DC_BIAS;
            data >>= OUTPUT_SCALE;
            /* Clip if needed. */
            if( INT16_MAX < data ) {
                data = INT16_MAX;
            }
        } else {
            data -= DC_BIAS;
            data >>= OUTPUT_SCALE;
            /* Clip if needed. */
            if( data < INT16_MIN ) {
                data = INT16_MIN;
            }
        }

        *out++ = (int16_t) data;

        /* Right sample --------------------- */

        /* Apply gain */
        data = (((int64_t) *r++) * (int64_t) gain_scale_factor) >> GAIN_SCALE;

        /* Convert from 32 bit to 16 bit */
        if( 0 < data ) {
            data += DC_BIAS;
            data >>= OUTPUT_SCALE;
            /* Clip if needed. */
            if( INT16_MAX < data ) {
                data = INT16_MAX;
            }
        } else {
            data -= DC_BIAS;
            data >>= OUTPUT_SCALE;
            /* Clip if needed. */
            if( data < INT16_MIN ) {
                data = INT16_MIN;
            }
        }

        *out++ = (int16_t) data;

        count--;
    }
}

/**
 *  Internal helper function that gets, populates and queues a request.
 *
 *  @param left the left audio channel
 *  @param right the right audio channel
 *  @param count the number of samples
 *  @param bitrate the bitrate of the data
 *  @param gain_scale_factor the gain to apply
 *  @param cb the callback to call
 *  @param data the data to send with the callback
 */
static void __queue_request( int32_t *left,
                             int32_t *right,
                             const size_t count,
                             const uint32_t bitrate,
                             const int32_t gain_scale_factor,
                             dsp_buffer_return_fct cb,
                             void *data )
{
    dsp_input_t *in;

    os_queue_receive( __input_idle, &in, WAIT_FOREVER );

    in->left = left;
    in->right = right;
    in->count = count;
    in->offset = 0;
    in->bitrate = bitrate;
    in->gain_scale_factor = gain_scale_factor;
    in->cb = cb;
    in->data = data;

    os_queue_send_to_back( __input_queued, &in, WAIT_FOREVER );
}
