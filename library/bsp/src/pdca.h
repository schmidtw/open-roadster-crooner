/*
 * Copyright (c) 2008-2009 Weston Schmidt
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
#ifndef __PDCA_H__
#define __PDCA_H__

#include <stdint.h>

#include "bsp_errors.h"

typedef enum {
    PDCA_ISR__TRANSFER_ERROR,
    PDCA_ISR__TRANSFER_COMPLETE,
    PDCA_ISR__RELOAD_COUNTER_ZERO
} pdca_isr_t;

/**
 *  Used to get the PDCA ISR name given the ISR number.
 *
 *  @note This macro is safe to pass macros into for the parameter
 *  id.
 *
 *  @param id the PDCA id number
 */
#define PDCA_GET_ISR_NAME( id )     __PDCA_GET_ISR_NAME( id )

/**
 *  Do not call this macro directly.
 */
#define __PDCA_GET_ISR_NAME( id )   ISR__PDCA_ ## id

/**
 *  Used to initialize a PDCA/DMA channel for transers.
 *
 *  @param channel the PDCA/DMA channel to setup
 *  @param options the options for this PDCA/DMA channel
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t pdca_channel_init( const uint8_t channel,
                                const uint8_t dest_pid,
                                const uint8_t transfer_data_size );

/**
 *  Used to add to the PDCA/DMA channel queue - does not start a transfer.
 *
 *  @param channel the PDCA/DMA channel to queue data for
 *  @param data the pointer to the data to be transefered
 *  @param size the size of the requested transfer in bytes
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 *      @retval BSP_PDCA_QUEUE_FULL
 */
bsp_status_t pdca_queue_buffer( const uint8_t channel,
                                volatile void *data,
                                const uint16_t size );

/**
 *  Used to cancel and reset the PDCA/DMA channel to it's initial state.
 *
 *  @param channel the PDCA/DMA channel to cancel & reset
 *  
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t pdca_disable( const uint8_t channel );

/**
 *  Used to enable an ISR for the PDCA/DMA channel.
 *
 *  @param channel the PDCA/DMA channel of interest
 *  @param isr the ISR to enable
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t pdca_isr_enable( const uint8_t channel, const pdca_isr_t isr );

/**
 *  Used to disable an ISR for the PDCA/DMA channel.
 *
 *  @param channel the PDCA/DMA channel of interest
 *  @param isr the ISR to disable
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t pdca_isr_disable( const uint8_t channel, const pdca_isr_t isr );

/**
 *  Used to clear the ISR causes due to the PDCA/DMA channel.
 *
 *  @param channel the PDCA/DMA channel of interest
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t pdca_isr_clear( const uint8_t channel );

/**
 *  Used to enable the PDCA/DMA transfer
 *
 *  @param channel the PDCA/DMA channel of interest
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t pdca_enable( const uint8_t channel );
#endif
