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
#ifndef __USART_H__
#define __USART_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <avr32/io.h>

#include "bsp_errors.h"
#include "gpio.h"

/** Parity Settings */
typedef enum {
    USART_PARITY__EVEN      = AVR32_USART_MR_PAR_EVEN,
    USART_PARITY__ODD       = AVR32_USART_MR_PAR_ODD,
    USART_PARITY__SPACE     = AVR32_USART_MR_PAR_SPACE,
    USART_PARITY__MARK      = AVR32_USART_MR_PAR_MARK,
    USART_PARITY__NONE      = AVR32_USART_MR_PAR_NONE,
    USART_PARITY__MULTIDROP = AVR32_USART_MR_PAR_MULTI
} usart_parity_t;

/** Stop Bits Settings */
typedef enum {
    USART_STOPBITS__1   = AVR32_USART_MR_NBSTOP_1,
    USART_STOPBITS__1_5 = AVR32_USART_MR_NBSTOP_1_5,
    USART_STOPBITS__2   = AVR32_USART_MR_NBSTOP_2,
} usart_stop_bits_t;

/** Channel Modes */
typedef enum {
    USART_MODE__NORMAL          = AVR32_USART_MR_CHMODE_NORMAL,
    USART_MODE__AUTO_ECHO       = AVR32_USART_MR_CHMODE_ECHO,
    USART_MODE__LOCAL_LOOPBACK  = AVR32_USART_MR_CHMODE_LOCAL_LOOP,
    USART_MODE__REMOTE_LOOPBACK = AVR32_USART_MR_CHMODE_REMOTE_LOOP
} usart_mode_t;

/** Data Bits */
typedef enum {
    USART_DATA_BITS__5  = 0,
    USART_DATA_BITS__6  = 1,
    USART_DATA_BITS__7  = 2,
    USART_DATA_BITS__8  = 3,
    USART_DATA_BITS__9  = 4
} usart_data_bits_t;

typedef struct {
    const uint32_t baudrate;
    const usart_data_bits_t data_bits;
    const usart_parity_t parity;
    const usart_stop_bits_t stop_bits;
    const usart_mode_t mode;
    const bool tx_only;
    const bool hw_handshake;
    const size_t map_size;
    const gpio_map_t *map;
} usart_options_t;

/**
 *  Used to reset a USART back to a safe & off state.
 *
 *  @param usart the USART to reset
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid arguement passed.
 */
bsp_status_t usart_reset( volatile avr32_usart_t *usart );

/**
 *  Used to initialize the USART into a normal operation node.
 *
 *  @param usart the USART to configure
 *  @param opt the options to set
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid arguement(s) passed.
 */
bsp_status_t usart_init_rs232( volatile avr32_usart_t *usart,
                               const usart_options_t *opt );

/**
 *  Used to determine if the USART is ready to transmit a character.
 *
 *  @param usart the USART to check
 *
 *  @return Status
 *      @retval true  Ready to transmit.
 *      @retval false Not ready to transmit or error.
 */
__inline__ bool usart_tx_ready( volatile avr32_usart_t *usart );

/**
 *  Used to write the character to the TX buffer if the hardware is
 *  ready to transmit.
 *
 *  @param usart the USART to write to
 *  @param c the character (up to 9 bits) to write
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_USART_TX_BUSY   USART is busy.
 */
bsp_status_t usart_write_char( volatile avr32_usart_t *usart, int c );

/**
 *  Used to read a character from the USART.
 *
 *  @param usart the USART to write to
 *  @param c the pointer to write the character to
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_USART_RX_ERROR  There was an error reading the data.
 *      @retval BSP_USART_RX_EMPTY  No character to read.
 */
bsp_status_t usart_read_char( volatile avr32_usart_t *usart, int *c );

#endif
