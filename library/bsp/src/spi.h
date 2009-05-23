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
#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>

#include <avr32/io.h>

#include "bsp_errors.h"

/**
 *  Used to reset an SPI module.
 *
 *  @param spi the SPI module to reset
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t spi_reset( volatile avr32_spi_t *spi );

/**
 *  Used to set the baudrate for a particular chip.
 *
 *  @param spi the SPI module to setup
 *  @param chip the chip index [0-3]
 *  @param max_frequency the maximum frequency in Hz to set the baudrate at
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t spi_set_baudrate( volatile avr32_spi_t *spi,
                               const uint8_t chip,
                               const uint32_t max_frequency );

/**
 *  Used to get the current baudrate for a particular chip.
 *
 *  @param spi the SPI module to inspect
 *  @param chip the chip index [0-3]
 *  @param baud_rate the current baud rate of the chip
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t spi_get_baudrate( volatile avr32_spi_t *spi,
                               const uint8_t chip,
                               uint32_t *baud_rate );

/**
 *  Used to enable an SPI module.
 *
 *  @param spi the SPI module to reset
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t spi_enable( volatile avr32_spi_t *spi );

/**
 *  Used to select a device on a SPI bus
 *
 *  @param spi the SPI bus
 *  @param chip the device number to select [0-3]
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t spi_select( volatile avr32_spi_t *spi, const uint8_t chip );

/**
 *  Used to unselect all devices on a SPI bus
 *
 *  @param spi the SPI bus
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t spi_unselect( volatile avr32_spi_t *spi );

/**
 *  Used to write data to the SPI port.
 *
 *  @note This is suspect code - needs better testing.
 *
 *  @param spi the SPI module
 *  @param data the data to write (the size is determined by the SPI settings)
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 *      @retval BSP_ERROR_TIMEOUT
 */
bsp_status_t spi_write( volatile avr32_spi_t *spi, const uint16_t data );

/**
 *  Used to write data to the SPI port & mark the device available to deselect.
 *
 *  @note This is suspect code - needs better testing.
 *
 *  @param spi the SPI module
 *  @param data the data to write (the size is determined by the SPI settings)
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 *      @retval BSP_ERROR_TIMEOUT
 */
bsp_status_t spi_write_last( volatile avr32_spi_t *spi, const uint16_t data );

/**
 *  Used to read data from the SPI port.
 *
 *  @note This is suspect code - needs better testing.
 *
 *  @param spi the SPI module
 *  @param data the pointer to where to write the data read
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 *      @retval BSP_ERROR_TIMEOUT
 */
bsp_status_t spi_read( volatile avr32_spi_t *spi, uint16_t *data );

/**
 *  Used to read data from the SPI port - optimized for 8 bits of data.
 *
 *  @note This is suspect code - needs better testing.
 *
 *  @param spi the SPI module
 *  @param data the pointer to where to write the data read
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 *      @retval BSP_ERROR_TIMEOUT
 */
bsp_status_t spi_read8( volatile avr32_spi_t *spi, uint8_t *data );

#endif
