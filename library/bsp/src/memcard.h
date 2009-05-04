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
#ifndef __MEMCARD_H__
#define __MEMCARD_H__

#include <stdbool.h>
#include <stdint.h>
#include <avr32/io.h>

#include "gpio.h"
#include "bsp_errors.h"

#define MEMCARD_MAX_PATH_LENGTH 16

/**
 *  Called by the BSP with the matching user_data from the struct.
 *  to lock the specified resource.
 *
 *  @param user_data The user data provided in mc_init().
 */
typedef void (*lock_fct)( volatile void *user_data );

/**
 *  Called by the BSP with the matching user_data from the struct.
 *  to unlock the specified resource.
 *
 *  @param user_data The user data provided in mc_init().
 */
typedef void (*unlock_fct)( volatile void *user_data );

/**
 *  Called by the BSP to cause the current task to sleep while
 *  retrieving data via DMA.
 *
 *  @param ms The number of milliseconds to sleep.
 */
typedef void (*sleep_fct)( const uint32_t ms );

/**
 *  Called by the BSP to suspend the current thread while
 *  waiting on data to be transfered.
 */
typedef void (*suspend_fct)( void );

/**
 *  Called by the BSP in an ISR to resume the thread waiting on
 *  data to be transfered.
 */
typedef void (*resume_fct)( void );

/**
 *  Used to initialize the memory card subsystem.  This <b>MUST</b>
 *  be called prior to using any other memory card functions.
 *
 *  @note The SPI bus(es) controlling the card(s) must be initialized.
 *
 *  @param cards The list of cards to setup (based on the hardware).
 *  @param count The number of cards in the list.
 *
 *  @return Status.
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t mc_init( lock_fct lock,
                      unlock_fct unlock,
                      sleep_fct sleep,
                      suspend_fct suspend,
                      resume_fct resume,
                      volatile void *user_data );

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
 *      @retval BSP_RETURN_OK           Success.
 *      @retval BSP_ERROR_PARAMETER     Invalid argument(s) passed.
 *      @retval BSP_MEMCARD_IN_USE      Card is already mounted.
 *      @retval BSP_MEMCARD_INIT_ERROR  The card did not respond properly.
 *      @retval BSP_MEMCARD_UNUSABLE    The card is not compatible.
 *      @retval BSP_ERROR_TIMEOUT       The card timed out during IO.
 *      @retval BSP_MEMCARD_ERROR_MODE  The card is in a generic error state.
 *      @retval BSP_MEMCARD_CRC_FAILURE The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 */
bsp_status_t mc_mount( void );

/**
 *  Used to unmount a memory card in a defined hardware slot.
 *
 *
 *  @return Status.
 *      @retval BSP_RETURN_OK           Success.
 *      @retval BSP_ERROR_PARAMETER     Invalid argument(s) passed.
 *      @retval BSP_MEMCARD_NOT_MOUNTED The card in the slot is not mounted.
 */
bsp_status_t mc_unmount( void );

/**
 *  Used to read a 512 byte block of memory from the card.
 *
 *  @param slot The hardware slot of the card to read from.
 *  @param lba The logical block address of the block to read.
 *  @param buffer A buffer to store the data read in.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK           Success.
 *      @retval BSP_ERROR_PARAMETER     Invalid argument(s) passed.
 *      @retval BSP_ERROR_TIMEOUT       The card timed out during IO.
 *      @retval BSP_MEMCARD_ERROR_MODE  The card is in a generic error state.
 *      @retval BSP_MEMCARD_CRC_FAILURE The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 *      @retval BSP_MEMCARD_NOT_MOUNTED The card in the slot is not mounted.
 */
bsp_status_t mc_read_block( const uint32_t lba, uint8_t *buffer );

/**
 *  Used to write a 512 byte block of memory to a card.
 *
 *  @param slot The hardware slot of the card to read from.
 *  @param lba The logical block address of the block to read.
 *  @param buffer A buffer to store the data read in.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK               Success.
 *      @retval BSP_ERROR_PARAMETER         Invalid argument(s) passed.
 *      @retval BSP_ERROR_TIMEOUT           The card timed out during IO.
 *      @retval BSP_MEMCARD_ERROR_MODE      The card is in a generic error state.
 *      @retval BSP_MEMCARD_NOT_SUPPORTED   The card does not support this operation.
 *      @retval BSP_MEMCARD_NOT_MOUNTED     The card in the slot is not mounted.
 */
bsp_status_t mc_write_block( const uint32_t lba, const uint8_t *buffer );

/**
 *  Used to get the block count for the current memory card.
 *
 *  @param blocks the output value to store the response
 *  @return Status
 *      @retval BSP_RETURN_OK           Success.
 *      @retval BSP_ERROR_PARAMETER     Invalid argument(s) passed.
 *      @retval BSP_MEMCARD_NOT_MOUNTED The card in the slot is not mounted.
 */
bsp_status_t mc_get_block_count( uint32_t *blocks );

#endif
