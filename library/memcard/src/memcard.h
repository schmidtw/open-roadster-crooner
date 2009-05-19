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
#ifndef __MEMCARD_H__
#define __MEMCARD_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    MC_RETURN_OK        = 0x0000,

    MC_ERROR_PARAMETER  = 0x0100,
    MC_ERROR_TIMEOUT    = 0x0101,
    MC_IN_USE           = 0x0102,
    MC_INIT_ERROR       = 0x0103,
    MC_UNUSABLE         = 0x0104,
    MC_ERROR_MODE       = 0x0105,
    MC_CRC_FAILURE      = 0x0106,
    MC_NOT_SUPPORTED    = 0x0107,
    MC_NOT_MOUNTED      = 0x0108,

    MC_STILL_WAITING    = 0x0200
} mc_status_t;

/**
 *  Used to initialize the memory card subsystem.  This <b>MUST</b>
 *  be called prior to using any other memory card functions.
 *
 *  @note The SPI bus(es) controlling the card(s) must be initialized.
 *
 *  @return Status.
 *      @retval MC_RETURN_OK       Success.
 *      @retval MC_ERROR_PARAMETER Invalid argument(s) passed.
 */
mc_status_t mc_init( void* (*fast_malloc_fn)(size_t) );

/**
 *  Used to determine if a memory card is present.
 *
 *  @return Status.
 *      @retval true if a card is present of the detection hardware
 *                   is not present
 *      @retval fase if no card is present
 */
bool mc_present( void );

/**
 *  Used to mount a memory card when present in the defined hardware slot.
 *
 *  @return Status.
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_PARAMETER  Invalid argument(s) passed.
 *      @retval MC_IN_USE           Card is already mounted.
 *      @retval MC_INIT_ERROR       The card did not respond properly.
 *      @retval MC_UNUSABLE         The card is not compatible.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_CRC_FAILURE      The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 */
mc_status_t mc_mount( void );

/**
 *  Used to unmount a memory card in a defined hardware slot.
 *
 *
 *  @return Status.
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_PARAMETER  Invalid argument(s) passed.
 *      @retval MC_NOT_MOUNTED      The card in the slot is not mounted.
 */
mc_status_t mc_unmount( void );

/**
 *  Used to read a 512 byte block of memory from the card.
 *
 *  @param slot The hardware slot of the card to read from.
 *  @param lba The logical block address of the block to read.
 *  @param buffer A buffer to store the data read in.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_PARAMETER  Invalid argument(s) passed.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_CRC_FAILURE      The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 *      @retval MC_NOT_MOUNTED The card in the slot is not mounted.
 */
mc_status_t mc_read_block( const uint32_t lba, uint8_t *buffer );

/**
 *  Used to write a 512 byte block of memory to a card.
 *
 *  @param slot The hardware slot of the card to read from.
 *  @param lba The logical block address of the block to read.
 *  @param buffer A buffer to store the data read in.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_PARAMETER  Invalid argument(s) passed.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_NOT_SUPPORTED    The card does not support this operation.
 *      @retval MC_NOT_MOUNTED      The card in the slot is not mounted.
 */
mc_status_t mc_write_block( const uint32_t lba, const uint8_t *buffer );

/**
 *  Used to get the block count for the current memory card.
 *
 *  @param blocks the output value to store the response
 *  @return Status
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_ERROR_PARAMETER  Invalid argument(s) passed.
 *      @retval MC_NOT_MOUNTED      The card in the slot is not mounted.
 */
mc_status_t mc_get_block_count( uint32_t *blocks );

#endif
