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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "commands.h"
#include "io.h"
#include "crc.h"
#include "command.h"
#include "timing-parameters.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define CSD_STRUCTURE_MASK          0xc0
#define CSD_STRUCTURE_SHIFT         6

#define CSD_TAAC_UNIT_MASK          0x03
#define CSD_TAAC_UNIT_SHIFT         0
#define CSD_TAAC_VALUE_MASK         0x78
#define CSD_TAAC_VALUE_SHIFT        3

#define CSD_TRAN_UNIT_MASK          0x03
#define CSD_TRAN_UNIT_SHIFT         0
#define CSD_TRAN_VALUE_MASK         0x78
#define CSD_TRAN_VALUE_SHIFT        3

#define CSD_READ_BL_LEN_MASK        0x0f
#define CSD_READ_BL_LEN_SHIFT       0

#define CSD_R2W_FACTOR_MASK         0x1c
#define CSD_R2W_FACTOR_SHIFT        2

#define CSD_VDD_R_CURR_MIN_MASK     0x38
#define CSD_VDD_R_CURR_MIN_SHIFT    3

#define CSD_VDD_R_CURR_MAX_MASK     0x07
#define CSD_VDD_R_CURR_MAX_SHIFT    0

#define CSD_VDD_W_CURR_MIN_MASK     0xe0
#define CSD_VDD_W_CURR_MIN_SHIFT    5

#define CSD_VDD_W_CURR_MAX_MASK     0x1c
#define CSD_VDD_W_CURR_MAX_SHIFT    2

#define SD_V1       0
#define SD_V2       1
#define MMC_V1_2    2

#define MC_BLOCK_START  0xfe

#define MC_CSD_BUFFER_SIZE  15

#define _D1(...)
#define _D2(...)

#ifdef CSD_DEBUG
#if (0 < CSD_DEBUG)
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif
#if (1 < CSD_DEBUG)
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static const uint8_t value_map[] = {  0, 10, 12, 13,  15, 20, 25, 30,
                                     35, 40, 45, 50,  55, 60, 70, 80 };

static const uint32_t min_current_map[] = {    500,
                                              1000,
                                              5000,
                                             10000,
                                             25000,
                                             35000,
                                             60000,
                                            100000 };

static const uint32_t max_current_map[] = {   1000,
                                              5000,
                                             10000,
                                             25000,
                                             35000,
                                             45000,
                                             80000,
                                            200000 };

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static uint64_t __csd_get_taac( const uint8_t *buffer );
static uint32_t __csd_get_nsac( const uint8_t *buffer );
static uint32_t __csd_get_tran_speed( const uint8_t *buffer );
static size_t __csd_get_read_block_len( const uint8_t *buffer );
#ifdef SUPPORT_WRITING
static size_t __csd_get_write_block_len( const uint8_t *buffer );
#endif
static uint64_t __csd_get_total_size( const uint8_t *buffer );
static uint32_t __csd_get_r2w_factor( const uint8_t *buffer );
static uint32_t __csd_get_min_r_ua_draw( const uint8_t *buffer );
static uint32_t __csd_get_max_r_ua_draw( const uint8_t *buffer );
static uint32_t __csd_get_min_w_ua_draw( const uint8_t *buffer );
static uint32_t __csd_get_max_w_ua_draw( const uint8_t *buffer );
static uint32_t __csd_get_nac_read( const uint8_t *buffer,
                                    const uint32_t clock );
static uint32_t __csd_get_nac_write( const uint8_t *buffer,
                                     const uint32_t clock );
static uint32_t __csd_get_nac_erase( const uint8_t *buffer,
                                     const uint32_t clock );
static uint32_t __get_csd_structure( const uint8_t *buffer );
static mc_status_t __get_csd_data( uint8_t *buffer );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See csd.h for details. */
mc_status_t mc_get_csd( mc_csd_t *csd, const uint32_t clock )
{
    mc_status_t status;
    uint8_t buffer[MC_CSD_BUFFER_SIZE];

    if( (NULL == csd) || (0 == clock) ) {
        return MC_ERROR_PARAMETER;
    }

    status = __get_csd_data( buffer );
    if( MC_RETURN_OK != status ) {
        return status;
    }

    csd->taac = __csd_get_taac( buffer );
    csd->nsac = __csd_get_nsac( buffer );
    csd->max_speed = __csd_get_tran_speed( buffer );
    csd->block_size = __csd_get_read_block_len( buffer );
    csd->total_size = __csd_get_total_size( buffer );
    csd->min_read = __csd_get_min_r_ua_draw( buffer );
    csd->max_read = __csd_get_max_r_ua_draw( buffer );
    csd->min_write = __csd_get_min_w_ua_draw( buffer );
    csd->max_write = __csd_get_max_w_ua_draw( buffer );
    csd->nac_read = __csd_get_nac_read( buffer, clock );
    csd->nac_write = __csd_get_nac_write( buffer, clock );
    csd->nac_erase = __csd_get_nac_erase( buffer, clock );

    _D1( "csd->taac: %llu ps\n", csd->taac );
    _D1( "csd->nsac: %lu cycles\n", csd->nsac );
    _D1( "csd->max_speed: %lu Hz\n", csd->max_speed );
    _D1( "csd->block_size: %lu bytes\n", csd->block_size );
    _D1( "csd->total_size: %llu bytes\n", csd->total_size );
    _D1( "csd->min_read: %lu uA\n", csd->min_read );
    _D1( "csd->max_read: %lu uA\n", csd->max_read );
    _D1( "csd->min_write: %lu uA\n", csd->min_write );
    _D1( "csd->max_write: %lu uA\n", csd->max_write );
    _D1( "csd->nac_read: %lu cycles\n", csd->nac_read );
    _D1( "csd->nac_write: %lu cycles\n", csd->nac_write );
    _D1( "csd->nac_erase: %lu cycles\n", csd->nac_erase );

    if( (0 == csd->taac) ||
        (0 == csd->max_speed) ||
        (0 == csd->block_size) ||
        (0 == csd->total_size) ||
        (0 == csd->nac_read) ||
        (0 == csd->nac_write) ||
        (0 == csd->nac_erase) )
    {
        return MC_UNUSABLE;
    }

    return MC_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to get the TAAC value in pico-seconds.
 *
 *  @param buffer the data to process
 *
 *  @return the TAAC in ps, or 0 on error
 */
static uint64_t __csd_get_taac( const uint8_t *buffer )
{
    uint64_t taac;
    uint32_t value, unit;

    const uint32_t unit_map[] = {        100,
                                        1000,
                                       10000,
                                      100000,
                                     1000000,
                                    10000000,
                                   100000000,
                                  1000000000 };

    value = (CSD_TAAC_VALUE_MASK & buffer[1]) >> CSD_TAAC_VALUE_SHIFT;
    unit  = (CSD_TAAC_UNIT_MASK & buffer[1]) >> CSD_TAAC_UNIT_SHIFT;

    taac = value_map[value];
    taac *= unit_map[unit];

    return taac;
}

/**
 *  Used to get the NSAC value in clock cycles.
 *
 *  @param buffer the data to process
 *
 *  @return the NSAC in clock cycles, or 0 on error
 */
static uint32_t __csd_get_nsac( const uint8_t *buffer )
{
    return (buffer[2] * 100);
}

/**
 *  Used to get the maximum transfer rate per one data line in kbit/s.
 *
 *  @param buffer the data to process
 *
 *  @return the maximum transfer rate in kbit/s, or 0 on error
 */
static uint32_t __csd_get_tran_speed( const uint8_t *buffer )
{
    uint32_t tran;
    uint32_t value, unit;

    const uint32_t unit_map[] = {    10000,
                                    100000,
                                   1000000,
                                  10000000,
                                         0,
                                         0,
                                         0,
                                         0 };

    unit = (CSD_TRAN_UNIT_MASK & buffer[3]) >> CSD_TRAN_UNIT_SHIFT;
    value = (CSD_TRAN_VALUE_MASK & buffer[3]) >> CSD_TRAN_VALUE_SHIFT;

    tran = value_map[value];
    tran *= unit_map[unit];

    return tran;
}

/**
 *  Used to get the read block length value in bytes.
 *
 *  @param buffer the data to process
 *
 *  @return number of bytes per read block, or 0 on error
 */
static size_t __csd_get_read_block_len( const uint8_t *buffer )
{
    size_t bl;

    bl = (CSD_READ_BL_LEN_MASK & buffer[5]) >> CSD_READ_BL_LEN_SHIFT;
    if( (9 <= bl) && (bl <= 11) ) {
        return (1 << bl);
    }

    return 0;
}

#ifdef SUPPORT_WRITING
/**
 *  Used to get the write block length value in bytes.
 *
 *  @param buffer the data to process
 *
 *  @return number of bytes per write block, or 0 on error
 */
static size_t __csd_get_write_block_len( const uint8_t *buffer )
{
    size_t bl;

    bl = ((0x03 & buffer[12]) << 2) | (0x03 & (buffer[13] >> 6));
    if( (9 <= bl) && (bl <= 11) ) {
        return (1 << bl);
    }

    return 0;
}
#endif

/**
 *  Used to get the total size of the card in bytes.
 *
 *  @param buffer the data to process
 *
 *  @return total size of the card in bytes, or 0 on error
 */
static uint64_t __csd_get_total_size( const uint8_t *buffer )
{
    uint64_t blocks;
    uint32_t structure;
    uint32_t block_size;

    blocks = 0;

    structure = __get_csd_structure( buffer );
    block_size = __csd_get_read_block_len( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        uint32_t c_size, c_size_mult;

        c_size = ((0x03 & buffer[6]) << 10) |
                    (buffer[7] << 2) |
                    (buffer[8] >> 6);

        c_size_mult = ((0x03 & buffer[9]) << 1) |
                        (buffer[10] >> 7);

        blocks = c_size;
        blocks++;
        blocks *= (1 << (2 + c_size_mult));
    } else if( SD_V2 == structure ) {
        uint64_t c_size;

        c_size = ((0x3f & buffer[7]) << 16) |
                    (buffer[8] << 8) | buffer[9];

        blocks = (1 + c_size) * 1024;
    }

    return (blocks * block_size);
}

/**
 *  Used to get the unit-less R2W_FACTOR value.
 *
 *  @param buffer the data to process
 *
 *  @return the unit-less R2W_FACTOR, or 0 on error
 */
static uint32_t __csd_get_r2w_factor( const uint8_t *buffer )
{
    uint32_t r2w;

    r2w = (CSD_R2W_FACTOR_MASK & buffer[12]) >> CSD_R2W_FACTOR_SHIFT;
    if( r2w < 6 ) {
        return (1 << r2w);
    }

    return 0;
}

/**
 *  Used to get the minimum current draw for reading in uA.
 *
 *  @param buffer the data to process
 *
 *  @return the minimum reading current draw in uA, or 0 on error
 */
static uint32_t __csd_get_min_r_ua_draw( const uint8_t *buffer )
{
    uint32_t ua;
    uint32_t structure;

    ua = 0;

    structure = __get_csd_structure( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        uint32_t current;
        current = (CSD_VDD_R_CURR_MIN_MASK & buffer[8]) >> CSD_VDD_R_CURR_MIN_SHIFT;
        ua = min_current_map[current];
    }

    return ua;
}

/**
 *  Used to get the maximum current draw for reading in uA.
 *
 *  @param buffer the data to process
 *
 *  @return the maximum reading current draw in uA, or 0 on error
 */
static uint32_t __csd_get_max_r_ua_draw( const uint8_t *buffer )
{
    uint32_t ua;
    uint32_t structure;

    ua = 0;

    structure = __get_csd_structure( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        uint32_t current;
        current = (CSD_VDD_R_CURR_MAX_MASK & buffer[8]) >> CSD_VDD_R_CURR_MAX_SHIFT;
        ua = max_current_map[current];
    }

    return ua;
}

/**
 *  Used to get the minimum current draw for writing in uA.
 *
 *  @param buffer the data to process
 *
 *  @return the minimum writing current drawi in uA, or 0 on error
 */
static uint32_t __csd_get_min_w_ua_draw( const uint8_t *buffer )
{
    uint32_t ua;
    uint32_t structure;

    ua = 0;

    structure = __get_csd_structure( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        uint32_t current;
        current = (CSD_VDD_W_CURR_MIN_MASK & buffer[9]) >> CSD_VDD_W_CURR_MIN_SHIFT;
        ua = min_current_map[current];
    }

    return ua;
}

/**
 *  Used to get the maximum current draw for writing in uA.
 *
 *  @param buffer the data to process
 *
 *  @return the maximum writing current drawi in uA, or 0 on error
 */
static uint32_t __csd_get_max_w_ua_draw( const uint8_t *buffer )
{
    uint32_t ua;
    uint32_t structure;

    ua = 0;

    structure = __get_csd_structure( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        uint32_t current;
        current = (CSD_VDD_W_CURR_MAX_MASK & buffer[9]) >> CSD_VDD_W_CURR_MAX_SHIFT;
        ua = max_current_map[current];
    }

    return ua;
}

/**
 *  Used to get the Nac value (in clock cycles) for reading.
 *
 *  @param buffer the data to process
 *
 *  @return the read Nac value in clock cycles, or 0 on error
 */
static uint32_t __csd_get_nac_read( const uint8_t *buffer,
                                    const uint32_t clock )
{
#ifdef USE_COMPLICATED_NAC_CALC
    uint32_t nac;
    uint32_t structure;
    uint64_t taac;
    uint32_t nsac;

    nac = 0;

    taac = __csd_get_taac( buffer );
    nsac = __csd_get_nsac( buffer );
    structure = __get_csd_structure( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        if( 0 < taac ) {
            uint64_t tmp;

            tmp = taac * ((uint64_t) clock);
            tmp += 999999999999ULL;
            tmp /= 1000000000000ULL;
            tmp += nsac;

            if( tmp < ((clock + 9) / 10) ) {
                nac = (uint32_t) tmp;
            } else {
                nac = (clock + 9) / 10;
            }

            if( 0 == nac ) {
                nac = 1;
            }
        }
    } else if( SD_V2 == structure ) {
        nac = (clock + 9) / 10;
    }

    return nac;
#else
    return ((clock + 9) / 10);
#endif
}

/**
 *  Used to get the Nac value (in clock cycles) for writing.
 *
 *  @param buffer the data to process
 *
 *  @return the write Nac value in clock cycles, or 0 on error
 */
static uint32_t __csd_get_nac_write( const uint8_t *buffer,
                                     const uint32_t clock )
{
    uint32_t nac;
    uint32_t structure;

    nac = 0;

    structure = __get_csd_structure( buffer );

    if( (SD_V1 == structure) || (MMC_V1_2 == structure) ) {
        nac = __csd_get_nac_read( buffer, clock );
        nac *= __csd_get_r2w_factor( buffer );
        if( ((clock + 3) / 4) < nac ) {
            nac = (clock + 3) / 4;
        }
    } else if( SD_V2 == structure ) {
        nac = (clock + 3) / 4;
    }

    return nac;
}

/**
 *  Used to get the Nac value (in clock cycles) for erasing.
 *
 *  @param buffer the data to process
 *
 *  @return the erase Nac value in clock cycles, or 0 on error
 */
static uint32_t __csd_get_nac_erase( const uint8_t *buffer,
                                     const uint32_t clock )
{
    return __csd_get_nac_write( buffer, clock );
}

/**
 *  Used to get the structure version.
 *
 *  @param buffer the data to process
 *
 *  @return 
 */
static uint32_t __get_csd_structure( const uint8_t *buffer )
{
    return ((CSD_STRUCTURE_MASK & buffer[0]) >> CSD_STRUCTURE_SHIFT);
}

/**
 *  Used to get the data from the card.
 *
 *  @param buffer where to put the data
 *
 *  @return Status
 *      @retval MC_ERROR_TIMEOUT
 *      @retval MC_ERROR_MODE
 *      @retval MC_CRC_FAILURE
 */
static mc_status_t __get_csd_data( uint8_t *buffer )
{
    mc_status_t status;
    uint8_t r1;
    uint8_t crc;
    int i;

    io_clean_select();

    status = mc_command( MCMD__SEND_CSD, 0, &r1 );

    if( MC_ERROR_TIMEOUT == status ) {
        goto error;
    }
    
    if( 0 != r1 ) {
        status = MC_ERROR_MODE;
        goto error;
    }

    for( i = 0; i < MC_Ncr; i++ ) {
        io_send_read( 0xff, buffer );
        if( MC_BLOCK_START == *buffer ) {
            goto csd_start;
        }
    }
    status = MC_ERROR_TIMEOUT;
    goto error;

csd_start:
    status = MC_CRC_FAILURE;

    for( i = 0; i < MC_CSD_BUFFER_SIZE; i++ ) {
        io_send_read( 0xff, &buffer[i] );
    }
    io_send_read( 0xff, &crc );

    if( 1 != (0x01 & crc) ) {
        goto error;
    }

#if (1 == MC_CHECK_CRCS)
    crc >>= 1;
    if( crc != crc7(buffer, MC_CSD_BUFFER_SIZE) ) {
        goto error;
    }
#endif
    status = MC_RETURN_OK;

error:
    io_clean_unselect();
    return status;
}

/*----------------------------------------------------------------------------*/
/*                                   Notes                                    */
/*----------------------------------------------------------------------------*/

/*
 *                                    Byte
 *    SD V2.0         SD V1. 0       Index      MMC V1.2
 *             +--------------------+------+
 *             |    csd_structure   |      |
 *             |    csd_structure   |      |
 *             +--------------------+      +------------------+
 *             |      reserved      |      |     spec_vers    |
 *             |      reserved      |  0   |     spec_vers    |
 *             |      reserved      |      |     spec_vers    |
 *             |      reserved      |      |     spec_vers    |
 *             |                    |      +------------------+
 *             |      reserved      |      |     reserved     |
 *             |      reserved      |      |     reserved     |
 *             +--------------------+------+------------------+
 *             |      reserved      |      |
 *             +--------------------+      |
 *             |     taac_value     |      |
 *             |     taac_value     |      |
 *             |     taac_value     |  1   |
 *             |     taac_value     |      |
 *             +--------------------+      |
 *             |      taac_unit     |      |
 *             |      taac_unit     |      |
 *             |      taac_unit     |      |
 *             +--------------------+------+
 *             |       nsac         |      |
 *             |       nsac         |      |
 *             |       nsac         |      |
 *             |       nsac         |      |
 *             |       nsac         |  2   |
 *             |       nsac         |      |
 *             |       nsac         |      |
 *             |       nsac         |      |
 *             +--------------------+------+
 *             |      reserved      |      |
 *             +--------------------+      |
 *             |     tran_value     |      |
 *             |     tran_value     |      |
 *             |     tran_value     |  3   |
 *             |     tran_value     |      |
 *             +--------------------+      |
 *             |      tran_unit     |      |
 *             |      tran_unit     |      |
 *             |      tran_unit     |      |
 *             +--------------------+------+
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |        ccc         |  4   |
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |                    +------+
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             |        ccc         |      |
 *             +--------------------+  5   |
 *             |    read_bl_len     |      |
 *             |    read_bl_len     |      |
 *             |    read_bl_len     |      |
 *             |    read_bl_len     |      |
 *             +--------------------+------+
 *             |  read_bl_partial   |      |
 *             +--------------------+      |
 *             | write_blk_misalign |      |
 *             +--------------------+      |
 *             |  read_blk_misalign |      |
 *             +--------------------+      |
 *             |      dsr_imp       |  6   |
 *  +----------+--------------------+      +------------------+
 *  | reserved |      reserved      |      |     reserved     |
 *  | reserved |      reserved      |      |     reserved     |
 *  |          +--------------------+      +------------------+
 *  | reserved |       c_size       |      |      c_size      |
 *  | reserved |       c_size       |      |      c_size      |
 *  |          |                    +------+                  |
 *  | reserved |       c_size       |      |      c_size      |
 *  | reserved |       c_size       |      |      c_size      |
 *  +----------+                    |      |                  |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |  c_size  |       c_size       |  7   |      c_size      |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |          |                    +------+                  |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |  c_size  |       c_size       |      |      c_size      |
 *  |          +--------------------+      +------------------+
 *  |  c_size  |   vdd_r_curr_min   |      |  vdd_r_curr_min  |
 *  |  c_size  |   vdd_r_curr_min   |      |  vdd_r_curr_min  |
 *  |  c_size  |   vdd_r_curr_min   |  8   |  vdd_r_curr_min  |
 *  |          +--------------------+      +------------------+
 *  |  c_size  |   vdd_r_curr_max   |      |  vdd_r_curr_max  |
 *  |  c_size  |   vdd_r_curr_max   |      |  vdd_r_curr_max  |
 *  |  c_size  |   vdd_r_curr_max   |      |  vdd_r_curr_max  |
 *  |          +--------------------+------+------------------+
 *  |  c_size  |   vdd_w_curr_min   |      |  vdd_w_curr_min  |
 *  |  c_size  |   vdd_w_curr_min   |      |  vdd_w_curr_min  |
 *  |  c_size  |   vdd_w_curr_min   |      |  vdd_w_curr_min  |
 *  |          +--------------------+      +------------------+
 *  |  c_size  |   vdd_w_curr_max   |      |  vdd_w_curr_max  |
 *  |  c_size  |   vdd_w_curr_max   |  9   |  vdd_w_curr_max  |
 *  |  c_size  |   vdd_w_curr_max   |      |  vdd_w_curr_max  |
 *  |          +--------------------+      +------------------+
 *  |  c_size  |     c_size_mult    |      |    c_size_mult   |
 *  |  c_size  |     c_size_mult    |      |    c_size_mult   |
 *  +----------+                    +------+                  |
 *  | reserved |     c_size_mult    |      |    c_size_mult   |
 *  +----------+--------------------+      +------------------+
 *             |    erase_blk_en    |      |  erase_grp_size  |
 *             +--------------------+      +------------------+
 *             |     sector_size    |      |  erase_grp_size  |
 *             |     sector_size    |  10  |  erase_grp_size  |
 *             |     sector_size    |      |  erase_grp_size  |
 *             |     sector_size    |      |  erase_grp_size  |
 *             |                    |      +------------------+
 *             |     sector_size    |      |  erase_grp_mult  |
 *             |     sector_size    |      |  erase_grp_mult  |
 *             |                    +------+                  |
 *             |     sector_size    |      |  erase_grp_mult  |
 *             +--------------------+      |                  |
 *             |     wp_grp_size    |      |  erase_grp_mult  |
 *             |     wp_grp_size    |      |  erase_grp_mult  |
 *             |                    |      +------------------+
 *             |     wp_grp_size    |  11  |    wp_grp_size   |
 *             |     wp_grp_size    |      |    wp_grp_size   |
 *             |     wp_grp_size    |      |    wp_grp_size   |
 *             |     wp_grp_size    |      |    wp_grp_size   |
 *             |     wp_grp_size    |      |    wp_grp_size   |
 *             +--------------------+------+------------------+
 *             |    wp_grp_enable   |      |
 *             +--------------------+      |
 *             |      reserved      |      |
 *             |      reserved      |      |
 *             +--------------------+      |
 *             |     r2w_factor     |  12  |
 *             |     r2w_factor     |      |
 *             |     r2w_factor     |      |
 *             +--------------------+      |
 *             |    write_bl_len    |      |
 *             |    write_bl_len    |      |
 *             |                    +------+
 *             |    write_bl_len    |      |
 *             |    write_bl_len    |      |
 *             +--------------------+      |
 *             |  write_bl_partial  |      |
 *             +--------------------+  13  |
 *             |      reserved      |      |
 *             |      reserved      |      |
 *             |      reserved      |      |
 *             |      reserved      |      |
 *             |                    |      +------------------+
 *             |      reserved      |      | content_prot_app |
 *             +--------------------+------+------------------+
 *             |   file_format_grp  |      |
 *             +--------------------+      |
 *             |        copy        |      |
 *             +--------------------+      |
 *             | perm_write_protect |      |
 *             +--------------------+      |
 *             |  tmp_write_protect |  14  |
 *             +--------------------+      |
 *             |     file_format    |      |
 *             |     file_format    |      |
 *             +--------------------+      +------------------+
 *             |      reserved      |      |        ecc       |
 *             |      reserved      |      |        ecc       |
 *             +--------------------+------+------------------+
 *             |         crc        |      |
 *             |         crc        |      |
 *             |         crc        |      |
 *             |         crc        |      |
 *             |         crc        |  15  |
 *             |         crc        |      |
 *             |         crc        |      |
 *             +--------------------+      |
 *             |      reserved      |      |
 *             +--------------------+------+
 */
