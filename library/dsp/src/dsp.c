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

#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include <bsp/boards/boards.h>
#include <bsp/usart.h>

#include <bsp/pdca.h>

#include "dsp.h"
#include "dac.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DSP_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define DSP_OUT_MSG_MAX     20
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
static xQueueHandle __output_idle;
static xQueueHandle __output_active;
static xQueueHandle __output_queued;
static xQueueHandle __output_idle_silence;
static dsp_output_t __output[DSP_OUT_MSG_MAX];

static xQueueHandle __input_idle;
static xQueueHandle __input_queued;
static dsp_input_t __input[DSP_IN_MSG_MAX];

static const dsp_output_t __silence[DSP_SILENCE_MSG_MAX] = {
    { .used = DSP_BUFFER_SIZE, .silence = true },
    { .used = DSP_BUFFER_SIZE, .silence = true } };

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
    portBASE_TYPE status;
    int32_t i;

    __output_idle = xQueueCreate( DSP_OUT_MSG_MAX, sizeof(dsp_output_t*) );
    if( NULL == __output_idle ) {
        goto failure_0;
    }
    __output_idle_silence = xQueueCreate( DSP_SILENCE_MSG_MAX, sizeof(dsp_output_t*) );
    if( NULL == __output_idle_silence ) {
        goto failure_0;
    }
    __output_active = xQueueCreate( 2, sizeof(dsp_output_t*) );
    if( NULL == __output_active ) {
        goto failure_1;
    }
    __output_queued = xQueueCreate( DSP_OUT_MSG_MAX, sizeof(dsp_output_t*) );
    if( NULL == __output_queued ) {
        goto failure_2;
    }

    __input_idle = xQueueCreate( DSP_IN_MSG_MAX, sizeof(dsp_input_t*) );
    if( NULL == __input_idle ) {
        goto failure_3;
    }
    __input_queued = xQueueCreate( DSP_IN_MSG_MAX, sizeof(dsp_input_t*) );
    if( NULL == __input_queued ) {
        goto failure_4;
    }

    for( i = 0; i < DSP_OUT_MSG_MAX; i++ ) {
        dsp_output_t *out = &__output[i];
        out->silence = false;
        xQueueSendToBack( __output_idle, &out, 0 );
    }

    for( i = 0; i < DSP_IN_MSG_MAX; i++ ) {
        dsp_input_t *in = &__input[i];
        xQueueSendToBack( __input_idle, &in, 0 );
    }

    for( i = 0; i < DSP_SILENCE_MSG_MAX; i++ ) {
        dsp_output_t *out = &__silence[i];
        xQueueSendToBack( __output_idle_silence, &out, 0 );
    }

    status = xTaskCreate( __dsp_task, ( signed portCHAR *) "DSP ",
                          DSP_TASK_STACK_SIZE, NULL, priority, NULL );

    if( pdPASS != status ) {
        goto failure_5;
    }

    dac_init( &__dac_buffer_complete, false );

    return DSP_RETURN_OK;

failure_5:
    vQueueDelete( __input_queued );
failure_4:
    vQueueDelete( __input_idle );
failure_3:
    vQueueDelete( __output_queued );
failure_2:
    vQueueDelete( __output_active );
failure_1:
    vQueueDelete( __output_idle );
failure_0:
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

    dac_set_sample_rate( 44100 );

    out = NULL;

    /* Start playing. */
    dac_start();

    while( 1 ) {
        dsp_input_t *in;

        if( pdTRUE == xQueueReceive( __input_queued, &in, portMAX_DELAY) ) {

            if( 0 == in->count ) {
                /* No more data - play silence. */
                if( NULL != out ) {
                    memset( &out->samples[out->used*2], 0,
                            (sizeof(int16_t)*2*(DSP_BUFFER_SIZE - out->used)) );
                    out->used = DSP_BUFFER_SIZE;
                    in->offset = 0;
                    xQueueSendToBack( __output_queued, &out, portMAX_DELAY );
                    out = NULL;
                }
            } else {
                while( in->offset < in->count ) {
                    if( NULL == out ) {
                        xQueueReceive( __output_idle, &out, portMAX_DELAY );
                        out->used = 0;
                    }

                    __process_samples( in, out );

                    if( DSP_BUFFER_SIZE == out->used ) {
                        xQueueSendToBack( __output_queued, &out, portMAX_DELAY );
                        out = NULL;
                    }
                }
            }

            /* We're done - send the buffer back & re-queue the message. */
            if( NULL != in->cb ) {
                (*in->cb)( in->left, in->right, in->data );
            }
            xQueueSendToBack( __input_idle, &in, portMAX_DELAY );
        }
    }
}

void isr_puts( const char *s )
{
    int i;
    for( i = 0; 0 != s[i]; i++ ) {
        while( false == usart_tx_ready(DEBUG_USART) ) { ; }

        usart_write_char( DEBUG_USART, s[i] );
    }
}

/**
 *  This is called when a buffer is completed & does the needed ISR cleanup
 *  and buffer management.
 */
__attribute__ ((__interrupt__))
static void __dac_buffer_complete( void )
{
    dsp_output_t *out;
    portBASE_TYPE status;
    portBASE_TYPE ignore;

    int i;

    //isr_puts( "__dac_buffer_complete()\n" );

    /* Move the transferred message to the idle queue. */
    status = xQueueReceiveFromISR( __output_active, &out, &ignore );
    if( pdTRUE == status ) {
        if( true == out->silence ) {
            xQueueSendToBackFromISR( __output_idle_silence, &out, &ignore );
        } else {
            xQueueSendToBackFromISR( __output_idle, &out, &ignore );
        }
    }

    for( i = 0; i < 2; i++ ) {
        xQueueHandle current;
        if( 0 == i ) {
            current = __output_queued;
        } else {
            current = __output_idle_silence;
        }

        while( (pdFALSE == xQueueIsQueueFullFromISR(__output_active)) &&
               (pdFALSE == xQueueIsQueueEmptyFromISR(current)) )
        {
            /* Queue the next pending buffer for playback. */
            status = xQueueReceiveFromISR( current, &out, &ignore );
            if( pdTRUE == status ) {
                bsp_status_t queue_status;
                /* This can't fail because one of the two buffers just
                 * became available the only other failure is parameter error. */
                queue_status = pdca_queue_buffer( PDCA_CHANNEL_ID_DAC,
                                                  out->samples, out->used << 2 );

                if( BSP_RETURN_OK == queue_status ) {
                    xQueueSendToBackFromISR( __output_active, &out, &ignore );
                } else {
                    isr_puts( "Bad pdca_queue_buffer() return value\n" );
                    xQueueSendToFrontFromISR( current, &out, &ignore );
                }
            }
        }
    }

    /* If there is a bubble in the audio, play silence */

    if( BSP_RETURN_OK != pdca_isr_clear(PDCA_CHANNEL_ID_DAC) ) {
        isr_puts( "pdca_isr_clear returned BSP_ERROR_PARAMETER\n" );
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
        data = (((int64_t) *in++) * gain_scale_factor) >> GAIN_SCALE;

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
        data = (((int64_t) *l++) * gain_scale_factor) >> GAIN_SCALE;

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
        data = (((int64_t) *r++) * gain_scale_factor) >> GAIN_SCALE;

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

    xQueueReceive( __input_idle, &in, portMAX_DELAY );

    in->left = left;
    in->right = right;
    in->count = count;
    in->offset = 0;
    in->bitrate = bitrate;
    in->gain_scale_factor = gain_scale_factor;
    in->cb = cb;
    in->data = data;

    xQueueSendToBack( __input_queued, &in, portMAX_DELAY );
}
