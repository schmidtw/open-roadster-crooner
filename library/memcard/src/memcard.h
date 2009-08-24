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
    MC_RETURN_OK            = 0x0000,

    MC_ERROR_PARAMETER      = 0x0100,
    MC_ERROR_TIMEOUT        = 0x0101,
    MC_INIT_ERROR           = 0x0102,
    MC_UNUSABLE             = 0x0103,
    MC_ERROR_MODE           = 0x0104,
    MC_CRC_FAILURE          = 0x0105,
    MC_NOT_SUPPORTED        = 0x0106,
    MC_NOT_MOUNTED          = 0x0107,
    MC_TOO_MANY_REGISTERED  = 0x0108,

    MC_STILL_WAITING        = 0x0200
} mc_status_t;

typedef enum {
    MC_CARD__INSERTED,
    MC_CARD__MOUNTING,
    MC_CARD__MOUNTED,
    MC_CARD__UNUSABLE,
    MC_CARD__REMOVED
} mc_card_status_t;

/**
 *  Called with a status update for the memory card/slot.
 *
 *  @param status the current status of any card in the slot
 */
typedef void (*card_status_fct)( const mc_card_status_t status );

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
 *  Used to register a callback notification when the status
 *  of the card changes.
 *
 *  @note The callback is based on a different thread.
 *
 *  @param card_status_fn the callback to call
 *
 *  @return Status
 *      @retval MC_RETURN_OK            Success.
 *      @retval MC_ERROR_PARAMETER      Invalid argument(s) passed.
 *      @retval MC_TOO_MANY_REGISTERED  Too many callbacks registered - failed 
 */
mc_status_t mc_register( card_status_fct card_status_fn );

/**
 *  Used to cancel callback notifications to this function.
 *
 *  @param card_status_fn the callback to cancel
 *
 *  @return Status
 *      @retval MC_RETURN_OK            Success.
 *      @retval MC_ERROR_PARAMETER      Invalid argument(s) passed.
 */
mc_status_t mc_cancel( card_status_fct card_status_fn );

/**
 *  Used to ask the current state of the memory card/slot.
 *
 *  @return Status.
 *      @retval MC_CARD__INSERTED   A card is inserted, but not mounted.
 *      @retval MC_CARD__MOUNTED    A card is inserted and is mounted.
 *      @retval MC_CARD__UNUSABLE   A card is inserted and is not useable.
 *      @retval MC_CARD__REMOVED    No card is present.
 */
mc_card_status_t mc_get_status( void );

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

#include <sys/reent.h>
#include <sys/stat.h>

int _file_fstat_r( struct _reent *reent, int fd, struct stat *st );
int _file_write_r( struct _reent *reent, int fd, void *buf, size_t len );
int _file_read_r( struct _reent *reent, int fd, void *buf, size_t len );
off_t _file_lseek_r( struct _reent *reent, int fd, off_t offset, int whence );
int _file_close_r( struct _reent *reent, int fd );
int _open_r( struct _reent *reent, const char *name, int flags, int mode );

#endif
