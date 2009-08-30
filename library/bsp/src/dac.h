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
#ifndef __DAC_H__
#define __DAC_H__

#include <stdbool.h>
#include <stdint.h>

#include "bsp_errors.h"

/**
 *  Used to initialize the DAC hardware.
 *
 *  @param complete_isr the isr function to call when a buffer is completed
 *  @param swap_channels if true, the L & R audio channels are swapped
 *
 *  @return Status
 *      @retval BSP_RETURN_OK               Success
 *      @retval BSP_ERROR_PARAMETER         Invalid parameter
 */
bsp_status_t dac_init( void (*complete_isr)(void), const bool swap_channels );

/**
 *  Used to start/resume the DAC hardware playing & unmute the output.
 */
void dac_start( void );

/**
 *  Used to pause the DAC hardware playing & mute the output.
 */
void dac_pause( void );

/**
 *  Used to stop the DAC hardware and mute the output.
 */
void dac_stop( void );

/**
 *  Used to set the output rate to match the sample rate unless
 *  the sample rate is not supported.
 *
 *  @param rate the sample rate desired
 *
 *  @return Status
 *      @retval BSP_RETURN_OK               Success
 *      @retval BSP_ERROR_UNSUPPORTED       Bitrate not supported
 */
bsp_status_t dac_set_sample_rate( const uint32_t rate );

/**
 *  Used to determine if the bitrate is supported without setting
 *  the bitrate.
 *
 *  @param rate the sample rate in question
 *
 *  @return true if supported, false otherwise
 */
bool dac_is_supported_bitrate( const uint32_t rate );
#endif
