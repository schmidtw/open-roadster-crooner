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

#include <stdint.h>

#include "commands.h"
#include "io.h"
#include "command.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MC_R1_IN_IDLE_STATE         (1 << 0)
#define MC_R1_ERASE_RESET           (1 << 1)
#define MC_R1_ILLEGAL_COMMAND       (1 << 2)
#define MC_R1_COM_CRC_ERROR         (1 << 3)
#define MC_R1_ERASE_SEQUENCE_ERROR  (1 << 4)
#define MC_R1_ADDRESS_ERROR         (1 << 5)
#define MC_R1_PARAMETER_ERROR       (1 << 6)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    uint32_t max;
    uint32_t min;
    uint8_t mask;
    int32_t index;
} mc_ocr_voltage_map_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See commands.h for detail. */
mc_status_t mc_go_idle_state( void )
{
    mc_status_t status;
    uint8_t r1;

    io_select();
    status = mc_command( MCMD__GO_IDLE_STATE, 0, &r1 );
    io_clean_unselect();

    if( (MC_RETURN_OK == status) && (0x01 == r1) ) {
        return MC_RETURN_OK;
    }

    return MC_ERROR_TIMEOUT;
}

/** See commands.h for detail. */
mc_status_t mc_send_if_cond( const uint32_t voltage, bool *legal_command )
{
    mc_status_t status;
    uint8_t r7[5];

    *legal_command = false;

    if( (voltage < 2700) || (3300 < voltage) ) {
        return MC_ERROR_PARAMETER;
    }

    /* Why 0x0001AA - 2.7-3.6 Volt range, plus recommended pattern
     * from SD 2.00 4.3.13 */
    status = mc_select_and_command( MCMD__SEND_IF_COND, 0x0001aa, MRT_R7, r7 );
    if( MC_RETURN_OK == status ) {
        if( MC_R1_IN_IDLE_STATE == r7[0] ) {
            if( (0x01 != r7[3]) || (0xaa != r7[4]) ) {
                return MC_UNUSABLE;
            }
            *legal_command = true;
        }
    }

    return MC_RETURN_OK;
}

/** See commands.h for detail. */
mc_status_t mc_read_ocr( mc_ocr_t *ocr )
{
    mc_status_t status;
    uint8_t r3[5];
    int i;

    const mc_ocr_voltage_map_t map[] = {
                    { .index = 2, .mask = 0x80, .max = 3600, .min = 3500 },
                    { .index = 2, .mask = 0x40, .max = 3500, .min = 3400 },
                    { .index = 2, .mask = 0x20, .max = 3400, .min = 3300 },
                    { .index = 2, .mask = 0x10, .max = 3300, .min = 3200 },
                    { .index = 2, .mask = 0x08, .max = 3200, .min = 3100 },
                    { .index = 2, .mask = 0x04, .max = 3100, .min = 3000 },
                    { .index = 2, .mask = 0x02, .max = 3000, .min = 2900 },
                    { .index = 2, .mask = 0x01, .max = 2900, .min = 2800 },
                    { .index = 3, .mask = 0x80, .max = 2800, .min = 2700 } };

    if( NULL == ocr ) {
        return MC_ERROR_PARAMETER;
    }

    status = mc_select_and_command( MCMD__READ_OCR, 0, MRT_R3, r3 );
    if( MC_RETURN_OK != status ) {
        return MC_ERROR_TIMEOUT;
    }

    ocr->busy = (0 == (0x80 & r3[1])) ? true : false;
    ocr->card_capacity_status = (0x40 == (0x40 & r3[1])) ? true : false;

    ocr->voltage_window_max = 2700;
    ocr->voltage_window_min = 3600;

    for( i = 0; i < sizeof(map)/sizeof(mc_ocr_voltage_map_t); i++ ) {
        if( map[i].mask == (map[i].mask & r3[map[i].index]) ) {
            if( ocr->voltage_window_max < map[i].max ) {
                ocr->voltage_window_max = map[i].max;
            }
            if( map[i].min < ocr->voltage_window_min ) {
                ocr->voltage_window_min = map[i].min;
            }
        }
    }

    return MC_RETURN_OK;
}

/** See commands.h for detail. */
mc_status_t mc_send_sd_op_cond( const bool support_hc, bool *card_ready )
{
    mc_status_t status;
    uint32_t cmd41_arg;
    uint8_t r1;

    if( NULL == card_ready ) {
        return MC_ERROR_PARAMETER;
    }

    cmd41_arg = (true == support_hc) ? 0x40000000 : 0;

    status = mc_select_and_command( MCMD__APP_CMD, 0, MRT_R1, &r1 );
    if( MC_RETURN_OK != status ) {
        return MC_ERROR_TIMEOUT;
    }

    status = mc_select_and_command( MCMD__SD_SEND_OP_COND, cmd41_arg,
                                    MRT_R1, &r1 );
    if( MC_RETURN_OK != status ) {
        return MC_ERROR_TIMEOUT;
    }

    *card_ready = (MC_R1_IN_IDLE_STATE == (MC_R1_IN_IDLE_STATE & r1)) ? true : false;

    return MC_RETURN_OK;
}

/** See commands.h for detail. */
mc_status_t mc_send_op_cond( const bool support_hc, bool *card_ready )
{
    mc_status_t status;
    uint32_t cmd1_arg;
    uint8_t r1;

    if( NULL == card_ready ) {
        return MC_ERROR_PARAMETER;
    }

    cmd1_arg = (true == support_hc) ? 0x40000000 : 0;

    status = mc_select_and_command( MCMD__SEND_OP_COND, cmd1_arg,
                                    MRT_R1, &r1 );
    if( MC_RETURN_OK != status ) {
        return MC_ERROR_TIMEOUT;
    }

    *card_ready = (MC_R1_IN_IDLE_STATE == (MC_R1_IN_IDLE_STATE & r1)) ? true : false;

    return MC_RETURN_OK;
}

/** See commands.h for detail. */
mc_status_t mc_set_block_size( const uint32_t block_size )
{
    mc_status_t status;
    uint8_t r1;

    status = mc_select_and_command( MCMD__SET_BLOCKLEN, block_size, MRT_R1, &r1 );
    if( MC_RETURN_OK != status ) {
        return MC_ERROR_TIMEOUT;
    }

    if( (0 == r1) || (MC_R1_IN_IDLE_STATE == r1) ) {
        return MC_RETURN_OK;
    }

    return MC_ERROR_PARAMETER;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
