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
#ifndef __ABDAC_H__
#define __ABDAC_H__

#include <stdbool.h>
#include <stdint.h>
#include <linked-list/linked-list.h>

#include "bsp_errors.h"

typedef enum {
    ABDAC_SAMPLE_RATE__48000,
    ABDAC_SAMPLE_RATE__44100,
    ABDAC_SAMPLE_RATE__32000,
    ABDAC_SAMPLE_RATE__24000,
    ABDAC_SAMPLE_RATE__22050,
    ABDAC_SAMPLE_RATE__16000,
    ABDAC_SAMPLE_RATE__12000,
    ABDAC_SAMPLE_RATE__11025,
    ABDAC_SAMPLE_RATE__8000
} abdac_sample_rate_t;

typedef struct {
    ll_node_t node;             /** A linked list node for internal use (may be used
                                 * externally when buffer isn't active). */
    volatile uint8_t *buffer;   /** The pointer to the buffer to play. */
    uint16_t size;              /** The size of the buffer in bytes. */
} abdac_node_t;

/**
 *  Used to handle buffers that have been consumed.
 *
 *  @note This called via an interrupt - global interrupts have
 *        been disabled, so no need to do that.  Because of this,
 *        make the function small & short.
 *
 *  @param node the buffer of the node that has been consumed
 *  @param last_buffer true if the last buffer has been reached and
 *                     audio output has been muted and shutdown, false
 *                     otherwise
 */
typedef void (*abdac_buffer_done_cb)( abdac_node_t *node, const bool last_buffer );

/**
 *  This initializes the ABDAC/DAC module.  This module uses the following
 *  resources:
 *      - The highest numbered PDCA/DMA channel on the device.
 *      - PLL1
 *      - AUDIO_DAC_MUTE_PIN (from the board configuration)
 *      - AUDIO_DAC_R_PIN (from the board configuration)
 *      - AUDIO_DAC_L_PIN (from the board configuration)
 *
 *  @param swap_channels swap the L & R channels
 *  @param buffer_done the callback to call when a buffer is now free
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t abdac_init( const bool swap_channels,
                         abdac_buffer_done_cb buffer_done );

/**
 *  Used to set the sample rate clock.  This sets PLL1 and the ABDAC/DAC
 *  general purpose clock.
 *
 *  @param rate the sample rate of the audio to play
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t abdac_set_sample_rate( const abdac_sample_rate_t rate );

/**
 *  Used to queue a buffer with samples to be transferred to the ABDAC/DAC.
 *
 *  @param node the node to queue
 *  @param last if this is the last buffer to output
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t abdac_queue_data( abdac_node_t *node, const bool last );

/**
 *  Used to add a short burst of silence (100 samples, stereo) to the
 *  current playback queue, and after this node is played, the audio
 *  is unmuted.  This prevents audio pops.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ABDAC_SILENCE_ALREADY_IN_USE
 */
bsp_status_t abdac_queue_silence( void );

/**
 *  Used to start the output of the ABDAC/DAC.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 */
bsp_status_t abdac_start( void );

/**
 *  Used to pause the output of the ABDAC/DAC.  Resume with abdac_resume().
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 */
bsp_status_t abdac_pause( void );

/**
 *  Used to resume output of the ABDAC/DAC after it has been paused
 *  with abdac_pause().
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 */
bsp_status_t abdac_resume( void );

/**
 *  Used to stop the output of the ABDAC/DAC.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 */
bsp_status_t abdac_stop( void );

/**
 *  Used to get the current underflow count of the current
 *  audio playback.  The count is automatically reset by
 *  abdac_start().
 *
 *  @return the number of underflow errors
 */
uint32_t abdac_get_underflow( void );

#endif
