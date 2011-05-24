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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdbool.h>
#include <stdint.h>

#include "memcard.h"

typedef struct {
    uint64_t taac;          /** The TAAC in ps. */
    uint32_t nsac;          /** The NSAC in cycles. */
    uint32_t max_speed;     /** The maximum transfer speed in Hz. */
    uint32_t block_size;    /** The current read block size in bytes. */
    uint64_t total_size;    /** Total size in bytes. */
    uint32_t min_read;      /** Minimum current draw (uA) during reading. */
    uint32_t max_read;      /** Maximum current draw (uA) during reading. */
    uint32_t min_write;     /** Minimum current draw (uA) during writing. */
    uint32_t max_write;     /** Maximum current draw (uA) during writing. */
    uint32_t nac_read;      /** The Nac value for reading. */
    uint32_t nac_write;     /** The Nac value for writing. */
    uint32_t nac_erase;     /** The Nac value for erasing. */
} mc_csd_t;

typedef struct {
    uint8_t manufacturer_id;
    char oem_id[3];
    char product_name[6];
    uint8_t major_version;
    uint8_t minor_version;
    uint32_t serial_number;
    char manufacture_date[16];
} mc_cid_t;

typedef struct {
    bool busy;
    bool card_capacity_status;
    uint32_t voltage_window_max;    /** In mV */
    uint32_t voltage_window_min;    /** In mV */
} mc_ocr_t;

typedef struct {
    bool card_is_locked;
    bool wp_erase_skip;
    bool error;
    bool cc_error;
    bool card_ecc_failed;
    bool wp_violation;
    bool erase_param;
    bool out_of_range;
    bool in_idle_state;
    bool erase_reset;
    bool illegal_command;
    bool com_crc_error;
    bool erase_sequence_error;
    bool address_error;
    bool parameter_error;
} mc_cs_t;

/**
 *  Used to send the GO_IDLE_STATE command to the card and process the
 *  response.
 *
 *  @note CMD0
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_go_idle_state( void );

/**
 *  Used to determine interface condition the card supports the voltage the
 *  system is running at.
 *
 *  @note CMD8
 *
 *  @param voltage the voltage the system is running at in mV
 *  @param legal_command the output if the command is legal (true), false
 *                       otherwise
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success, the card is compatible
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_UNUSABLE         Failure due to unusable card
 */
mc_status_t mc_send_if_cond( const uint32_t voltage, bool *legal_command );

/**
 *  Used to retreive the Operation Condition Register (OCR) from the card.
 *
 *  @note CMD58
 *
 *  @param ocr the structure to populate with information on a success
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success, the card is compatible
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_read_ocr( mc_ocr_t *ocr );

/**
 *  Used to send host support for high capacity information so the card
 *  can operate in that mode if supported.
 *
 *  @note CMD55 + ACMD41
 *
 *  @param support_hc if the host supports high capacity mode
 *  @param card_ready the output of if the card is ready
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success, the card is compatible
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_send_sd_op_cond( const bool support_hc, bool *card_ready );

/**
 *  Used to send host support for high capacity information so the card
 *  can operate in that mode if supported.
 *
 *  @note CMD1
 *
 *  @param support_hc if the host supports high capacity mode
 *  @param card_ready the output of if the card is ready
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success, the card is compatible
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_send_op_cond( const bool support_hc, bool *card_ready );

/**
 *  Used to set the card blocksize for reading and writing.
 *
 *  @note CMD16
 *
 *  @param block_size the size of the memory blocks in bytes
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success, the card is compatible
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_set_block_size( const uint32_t block_size );

/**
 *  Used to get the CSD value for use by the other csd_* functions.
 *
 *  @note This needs to be called each time the clock changes
 *
 *  @note CMD9
 *
 *  @param csd    The data to populate with the csd value on success.  Must
 *                not be NULL.
 *  @param pba_hz The current clock rate (in Hz) to use for timing calculations.
 *                Must be greater than 0.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 *      @retval MC_ERROR_MODE       Failure due to bad card response
 *      @retval MC_UNUSABLE         Failure due to unusable card
 *      @retval MC_CRC_FAILURE      Failure due to CRC error
 */
mc_status_t mc_get_csd( mc_csd_t *csd, const uint32_t clock );

/**
 *  Enable or disable CRC checking for the chip & host.
 *
 *  @note CMD59
 *
 *  @param enable If true, enables CRC checking, false disables
 *  @return Status
 *      @retval MC_RETURN_OK        Success, the card is compatible
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_set_crc( const bool enable );

/**
 *  Used to get the card identification values.
 *
 *  @note CMD10
 *
 *  @param cid The data structure to populate with the cid values on success.  Must
 *             not be NULL.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 *      @retval MC_ERROR_MODE       Failure due to bad card response
 *      @retval MC_UNUSABLE         Failure due to unusable card
 *      @retval MC_CRC_FAILURE      Failure due to CRC error
 */
mc_status_t mc_get_cid( mc_cid_t *cid );

/**
 *  Used to enable or disable CRC values.
 *
 *  @note CMD59
 *
 *  @param enabled If CRC mode is enabled (true) or not (false)
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_set_crc_mode( const bool enabled );

/**
 *  Used to get the card status
 *
 *  @note CMD13
 *
 *  @status The status data structure to populate with the card status.
 *
 *  @return Status
 *      @retval MC_RETURN_OK        Success
 *      @retval MC_ERROR_PARAMETER  Failure due to a bad parameter
 *      @retval MC_ERROR_TIMEOUT    Failure due to timeout
 */
mc_status_t mc_get_card_status( mc_cs_t *status );
#endif
