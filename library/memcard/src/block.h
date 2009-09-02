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
#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <stdint.h>

#include "memcard.h"

/**
 *  Used to initialize the memory block reading subsystem.
 *
 *  @return Status.
 *      @retval MC_RETURN_OK       Success.
 *      @retval MC_ERROR_PARAMETER Invalid argument(s) passed.
 */
mc_status_t block_init( void* (*fast_malloc_fn)(size_t) );

/**
 *  Used to read a 512 byte block of memory from the card.
 *
 *  @note No error checking is done on the input or states.
 *
 *  @param slot The hardware slot of the card to read from.
 *  @param lba The logical block address of the block to read.
 *  @param buffer A buffer to store the data read in.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_CRC_FAILURE      The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 *      @retval MC_NOT_SUPPORTED    The card does not support this operation.
 */
mc_status_t block_read( const uint32_t lba, uint8_t *buffer );

/**
 *  Used to write a 512 byte block of memory to a card.
 *
 *  @note No error checking is done on the input or states.
 *
 *  @param slot The hardware slot of the card to read from.
 *  @param lba The logical block address of the block to read.
 *  @param buffer A buffer to store the data read in.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_NOT_SUPPORTED    The card does not support this operation.
 */
mc_status_t block_write( const uint32_t lba, const uint8_t *buffer );

/**
 *  Used to cancel any pending IO operations & is called by an ISR.
 */
void block_isr_cancel( void );

#endif
