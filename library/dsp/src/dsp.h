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
#ifndef __DSP_H__
#define __DSP_H__

typedef enum {
    DSP_RETURN_OK,
    DSP_PARAMETER_ERROR,
    DSP_RESOURCE_ERROR,
    DSP_UNSUPPORTED_BITRATE
} dsp_status_t;

typedef enum {
    DSP_CMD__STOP,
    DSP_CMD__PLAY,
    DSP_CMD__PAUSE
} dsp_cmd_t;

typedef void (*dsp_buffer_return_fct)( int32_t *left,
                                       int32_t *right,
                                       void *data );

/**
 *  Used to initialize the DSP system.
 *
 *  @param priority the priority of the task
 *
 *  @return Status
 *      @retval DSP_RETURN_OK       Success
 *      @retval DSP_PARAMETER_ERROR Invalid parameter
 */
dsp_status_t dsp_init( const uint32_t priority );

/**
 *  Used to control the DSP.
 *
 *  @param cmd the command to send to the dsp system
 *
 *  @return Status
 *      @retval DSP_RETURN_OK       Success
 */
dsp_status_t dsp_control( const dsp_cmd_t cmd );

/**
 *  Used to determine the gain for a peak/gain pair.
 *
 *  @note peak & gain of 0.0 means no gain is applied.
 *
 *  @param peak the peak parameter of the replay gain
 *  @param gain the gain parameter of the replay gain
 *
 *  @return the scale factor to use when calling dsp_queue_data()
 */
int32_t dsp_determine_scale_factor( const double peak, const double gain );

/**
 *  Used to queue new samples for playback.
 *
 *  @note Takes ownership of the buffer until the buffer
 *        is returned via the callback function.
 *  @note cb is never called from an ISR.
 *
 *  @param left the buffer containing the left channel data to queue
 *  @param right the buffer containing the right channel data to queue
 *  @param count the number of samples contained in the buffers
 *  @param bitrate the bitrate of the sample set
 *  @param gain_scale_factor the scale factor gotten by calling
 *                           dsp_determine_scale_factor()
 *  @param cb the callback to call when done with the buffer
 *  @param data caller data returned as a parameter to the callback
 *              when this data is done
 *
 *  @return Status
 *      @retval DSP_RETURN_OK       Success
 *      @retval DSP_PARAMETER_ERROR Invalid parameter
 */
dsp_status_t dsp_queue_data( int32_t *left,
                             int32_t *right,
                             const size_t count,
                             const uint32_t bitrate,
                             const int32_t gain_scale_factor,
                             dsp_buffer_return_fct cb,
                             void *data );

/**
 *  Used to indicate that no more data is to be played at this time.
 *
 *  @note The callback will be called once the end of the current
 *        stream of audio is reached.
 *
 *  @param cb the callback to call when done with the buffer
 *  @param data caller data returned as a parameter to the callback
 *              when this data is done
 *
 */
void dsp_data_complete( dsp_buffer_return_fct cb,
                        void *data );
#endif
