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

#include <bsp/pdca.h>

#include "dsp.h"
#include "dac.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DSP_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 1000)
#define DSP_OUT_MSG_MAX     40
#define DSP_IN_MSG_MAX      10
#define DSP_BUFFER_SIZE     512

#define GAIN_SCALE      8
#define OUTPUT_SCALE    12
#define DC_BIAS         (1 << (OUTPUT_SCALE - 1))
#define MIN(a,b)        ((a) < (b)) ? (a) : (b)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    int16_t samples[DSP_BUFFER_SIZE * 2];
    size_t used;
    bool signal_played;
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
static xSemaphoreHandle __signal_played;

static xQueueHandle __output_idle;
static xQueueHandle __output_active;
static xQueueHandle __output_queued;
static dsp_output_t __output[DSP_OUT_MSG_MAX];

static xQueueHandle __input_idle;
static xQueueHandle __input_queued;
static dsp_input_t __input[DSP_IN_MSG_MAX];

static volatile uint32_t __dac_underflow_count;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __dsp_task( void *params );
static void __queue_silence( void );
__attribute__ ((__interrupt__)) static void __dac_buffer_complete( void );
__attribute__ ((__interrupt__)) static void __dac_underrun( void );
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

    __dac_underflow_count = 0;

    vSemaphoreCreateBinary( __signal_played );

    __output_idle = xQueueCreate( DSP_OUT_MSG_MAX, sizeof(dsp_output_t*) );
    if( NULL == __output_idle ) {
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
        xQueueSendToBack( __output_idle, &out, 0 );
    }

    for( i = 0; i < DSP_IN_MSG_MAX; i++ ) {
        dsp_input_t *in = &__input[i];
        xQueueSendToBack( __input_idle, &in, 0 );
    }

    status = xTaskCreate( __dsp_task, ( signed portCHAR *) "DSP ",
                          DSP_TASK_STACK_SIZE, NULL, priority, NULL );

    if( pdPASS != status ) {
        goto failure_5;
    }

    dac_init( &__dac_buffer_complete, &__dac_underrun, false );

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
    bool playing_music;

    dac_set_sample_rate( 44100 );

    /* We're always playing - queue a bunch of silence */
    __queue_silence();

    /* Move the two active playing data samples to the active list
     * and add them to the DMA controller list. */
    xQueueReceive( __output_queued, &out, 0 );
    xQueueSendToBack( __output_active, &out, 0 );
    pdca_queue_buffer( PDCA_CHANNEL_ID_DAC, out->samples, out->used << 2 );

    xQueueReceive( __output_queued, &out, 0 );
    xQueueSendToBack( __output_active, &out, 0 );
    pdca_queue_buffer( PDCA_CHANNEL_ID_DAC, out->samples, out->used << 2 );

    playing_music = false;
    out = NULL;

    /* Start playing. */
    dac_start();

    while( 1 ) {
        dsp_input_t *in;

        if( pdTRUE == xQueueReceive( __input_queued, &in, TASK_DELAY_MS(2)) ) {

            if( 0 == in->count ) {
                /* No more data - play silence. */
                if( NULL != out ) {
                    memset( &out->samples[out->used*2], 0,
                            (sizeof(int16_t)*2*(DSP_BUFFER_SIZE - out->used)) );
                    out->used = DSP_BUFFER_SIZE;
                    out->signal_played = false;
                    in->offset = 0;
                    xQueueSendToBack( __output_queued, &out, portMAX_DELAY );
                }
                playing_music = false;
                __queue_silence();
            } else {
                while( in->offset < in->count ) {
                    if( NULL == out ) {
                        xQueueReceive( __output_idle, &out, portMAX_DELAY );
                        out->used = 0;
                        out->signal_played = false;
                    }

                    __process_samples( in, out );

                    if( DSP_BUFFER_SIZE == out->used ) {
                        xQueueSendToBack( __output_queued, &out, portMAX_DELAY );
                        out = NULL;
                    }
                }
                playing_music = true;
            }

            /* We're done - send the buffer back & re-queue the message. */
            if( NULL != in->cb ) {
                (*in->cb)( in->left, in->right, in->data );
            }
            xQueueSendToBack( __input_idle, &in, portMAX_DELAY );
        } else {
            if( false == playing_music ) {
                /* No new samples - play silence */
                __queue_silence();
            }
        }
    }
}

static void __queue_silence( void )
{
    dsp_output_t *out;

    /* We're always playing - queue a bunch of silence */
    while( pdTRUE == xQueueReceive(__output_idle, &out, 0) ) {
        memset( out->samples, 0, sizeof(int16_t)*2*DSP_BUFFER_SIZE );
        out->used = DSP_BUFFER_SIZE;
        out->signal_played = false;
        xQueueSendToBack( __output_queued, &out, portMAX_DELAY );
    }
}

/**
 *  This is called when a buffer is completed & does the needed ISR cleanup
 *  and buffer management.
 */
__attribute__ ((__interrupt__))
static void __dac_buffer_complete( void )
{
    bool disable_dac;
    dsp_output_t *out;
    portBASE_TYPE status;
    portBASE_TYPE ignore;

    disable_dac = true;

    /* Move the transferred message to the idle queue. */
    status = xQueueReceiveFromISR( __output_active, &out, &ignore );
    if( pdTRUE == status ) {
        xQueueSendToBackFromISR( __output_idle, &out, &ignore );

        if( out->signal_played ) {
            xSemaphoreGiveFromISR( __signal_played, &ignore );
        }
        disable_dac = false;
    }

    /* Queue the next pending buffer for playback. */
    status = xQueueReceiveFromISR( __output_queued, &out, &ignore );
    if( pdTRUE == status ) {
        /* This can't fail because one of the two buffers just
         * became available the only other failure is parameter error. */
        pdca_queue_buffer( PDCA_CHANNEL_ID_DAC, out->samples, out->used << 2 );

        xQueueSendToBackFromISR( __output_active, &out, &ignore );

        disable_dac = false;
    }

    pdca_isr_clear( PDCA_CHANNEL_ID_DAC );

    if( true == disable_dac ) {
        dac_pause();
    }
}

/**
 *  This is called when there is an audio underrun.
 */
__attribute__ ((__interrupt__))
static void __dac_underrun( void )
{
    dac_clear_underrun();
    __dac_underflow_count++;
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
        data += DC_BIAS;
        data >>= OUTPUT_SCALE;

        /* Clip the sample */
        if( (int16_t) data != data ) {
            data = 0x7fff ^ (data >> 31);
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
    int32_t last[3];

    last[0] = 0;
    last[1] = 0;
    last[2] = 0;

    while( 0 < count ) {
        register int32_t data;

        /* Left sample ---------------------- */

        /* Apply gain */
        data = (((int64_t) *l++) * gain_scale_factor) >> GAIN_SCALE;

        /* Convert from 32 bit to 16 bit */
        data += DC_BIAS;
        data >>= OUTPUT_SCALE;

        last[0] = last[1];
        last[1] = last[2];
        last[2] = data;

        /* Clip the sample */
        if( (int16_t) data != data ) {
            data = 0x7fff ^ (data >> 31);
        }

        *out++ = (int16_t) data;

        /* Right sample --------------------- */

        /* Apply gain */
        data = (((int64_t) *r++) * gain_scale_factor) >> GAIN_SCALE;

        /* Convert from 32 bit to 16 bit */
        data += DC_BIAS;
        data >>= OUTPUT_SCALE;

        last[0] = last[1];
        last[1] = last[2];
        last[2] = data;

        /* Clip the sample */
        if( (int16_t) data != data ) {
            data = 0x7fff ^ (data >> 31);
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
