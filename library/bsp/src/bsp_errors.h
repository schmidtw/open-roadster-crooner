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
#ifndef __BSP_ERRORS_H__
#define __BSP_ERRORS_H__

typedef enum {
    BSP_RETURN_OK                       = 0x0000,
    BSP_ERROR_TIMEOUT                   = 0x0001,
    BSP_ERROR_PARAMETER                 = 0x0002,
    BSP_ERROR_CLOCK_NOT_SET             = 0x0003,
    BSP_MODE_FAULT                      = 0x0004,
    BSP_ERROR_UNSUPPORTED               = 0x0005,

    BSP_USART_TX_BUSY                   = 0x0100,
    BSP_USART_RX_EMPTY                  = 0x0101,
    BSP_USART_RX_ERROR                  = 0x0102,

    BSP_SPI_MODE_FAULT                  = 0x0200,

    BSP_PDCA_QUEUE_FULL                 = 0x0400,

    BSP_ABDAC_SILENCE_ALREADY_IN_USE    = 0x0500
} bsp_status_t;

#endif
