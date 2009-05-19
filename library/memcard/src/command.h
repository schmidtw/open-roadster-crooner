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

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdint.h>
#include "memcard.h"

/** See the SD/MMC definition for the source of these values. */
typedef enum {
    MCMD__GO_IDLE_STATE     = 0,
    MCMD__SEND_OP_COND      = 1,
    MCMD__SEND_IF_COND      = 8,
    MCMD__SEND_CSD          = 9,
    MCMD__SET_BLOCKLEN      = 16,
    MCMD__SD_SEND_OP_COND   = 41,
    MCMD__APP_CMD           = 55,
    MCMD__READ_OCR          = 58
} mc_cmd_type_t;

/**
 *  The values of these are also the lengths, so despite what looks
 *  like an error of 2 enumeration types with the value, it is correct. */
typedef enum {
    MRT_R1 = 1,
    MRT_R3 = 5,
    MRT_R7 = 5
} mc_response_type_t;

/**
 *  Used to to send a command and wait for the first character of the
 *  response.
 *
 *  @note Does not select of unselect the card.
 *
 *  @param command the command to send to the card
 *  @param argument the arguement sent with the command to the card
 *  @param response the output of the first character of the response, must
 *                  not be NULL
 *
 *  @return Status
 *      @retval MC_RETURN_OK
 *      @retval MC_ERROR_TIMEOUT
 */
mc_status_t mc_command( const mc_cmd_type_t command,
                        const uint32_t argument,
                        uint8_t *response );

/**
 *  Used to cleanly select the card, send the command, collect the response
 *  and cleanly unselect the card.
 *
 *  @param command the command to send to the card
 *  @param argument the arguement sent with the command to the card
 *  @param response the output of the first character of the response, must
 *                  not be NULL
 *
 *  @return Status
 *      @retval MC_RETURN_OK
 *      @retval MC_ERROR_TIMEOUT
 */
mc_status_t mc_select_and_command( const mc_cmd_type_t command,
                                   const uint32_t argument,
                                   const mc_response_type_t type,
                                   uint8_t *response );
#endif
