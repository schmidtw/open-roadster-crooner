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
#ifndef __IO_H__
#define __IO_H__

#include <stdint.h>

#include <bsp/bsp_errors.h>

/**
 *  Used to send data to the memory card via the SPI port.
 *
 *  @param out the byte of data to send
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_TIMEOUT
 */
inline bsp_status_t io_send( const uint8_t out );

/**
 *  Used to send dummy (0xff) data to the memory card via the SPI port.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_TIMEOUT
 */
inline bsp_status_t io_send_dummy( void );

/**
 *  Used to read data from the memory card via the SPI port.
 *
 *  @param in the address to read the data into on a success
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_TIMEOUT
 */
inline bsp_status_t io_read( uint8_t *in );

/**
 *  Used to select the memory card via the "chip select" line.
 *
 *  @note Does NOT send any data or clock cycles.
 */
inline void io_select( void );

/**
 *  Used to cleanly select the memory card via the "chip select" line.
 *
 *  @note This sends MC_Ncs clock cycles prior to the selection
 *        to make the card happy.
 */
inline void io_clean_select( void );

/**
 *  Used to un-select the memory card via the "chip select" line.
 *
 *  @note Does NOT send any data or clock cycles.
 */
inline void io_unselect( void );

/**
 *  Used to cleanly un-select the memory card via the "chip select" line.
 *
 *  @note This sends MC_Nec clock cycles prior to the un-selection
 *        to make the card happy.
 *  @note This sends 8 clock cycles of after the un-selection
 *        to make the card happy.
 */
inline void io_clean_unselect( void );

/**
 *  Used to send data and read the response from the memory card via the
 *  SPI port.
 *
 *  @param out the data to send to the card
 *  @param in the address to read the data into on a success
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_TIMEOUT
 */
inline bsp_status_t io_send_read( const uint8_t out, uint8_t *in );

/**
 *  Used to disable or reset the port to a "ready to receive a card" state.
 */
void io_disable( void );

/**
 *  Used to enable the SPI port for the io code.
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t io_enable( void );

/**
 *  Used to wake the card up by sending clock cycles.
 */
void io_wakeup_card( void );
#endif
