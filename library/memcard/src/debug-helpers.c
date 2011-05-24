/*
 * Copyright (c) 2011  Weston Schmidt
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

#include <stdio.h>

#include "debug-helpers.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

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
void print_cid( mc_cid_t *cid )
{
    if( NULL != cid ) {
        printf( "cid->manufacturer_id:  0x%08x\n", cid->manufacturer_id );
        printf( "cid->oem_id:           '%s'\n", cid->oem_id );
        printf( "cid->product_name:     '%s'\n", cid->product_name );
        printf( "cid->version:          %u.%u\n", cid->major_version, cid->minor_version );
        printf( "cid->serial_number:    0x%08lx\n", cid->serial_number );
        printf( "cid->manufacture_date: '%s'\n", cid->manufacture_date );
    }
}

void print_csd( mc_csd_t *csd )
{
    if( NULL != csd ) {
        printf( "csd->taac: %llu ps\n", csd->taac );
        printf( "csd->nsac: %lu cycles\n", csd->nsac );
        printf( "csd->max_speed: %lu Hz\n", csd->max_speed );
        printf( "csd->block_size: %lu bytes\n", csd->block_size );
        printf( "csd->total_size: %llu bytes\n", csd->total_size );
        printf( "csd->min_read: %lu uA\n", csd->min_read );
        printf( "csd->max_read: %lu uA\n", csd->max_read );
        printf( "csd->min_write: %lu uA\n", csd->min_write );
        printf( "csd->max_write: %lu uA\n", csd->max_write );
        printf( "csd->nac_read: %lu cycles\n", csd->nac_read );
        printf( "csd->nac_write: %lu cycles\n", csd->nac_write );
        printf( "csd->nac_erase: %lu cycles\n", csd->nac_erase );
    }
}

void print_card_status( mc_cs_t *status )
{
    if( NULL != status ) {
        printf( "status->card_is_locked: %s\n", (true == status->card_is_locked) ? "true" : "false" );
        printf( "status->wp_erase_skip: %s\n", (true == status->wp_erase_skip) ? "true" : "false" );
        printf( "status->error: %s\n", (true == status->error) ? "true" : "false" );
        printf( "status->cc_error: %s\n", (true == status->cc_error) ? "true" : "false" );
        printf( "status->card_ecc_failed: %s\n", (true == status->card_ecc_failed) ? "true" : "false" );
        printf( "status->wp_violation: %s\n", (true == status->wp_violation) ? "true" : "false" );
        printf( "status->erase_param: %s\n", (true == status->erase_param) ? "true" : "false" );
        printf( "status->out_of_range: %s\n", (true == status->out_of_range) ? "true" : "false" );
        printf( "status->in_idle_state: %s\n", (true == status->in_idle_state) ? "true" : "false" );
        printf( "status->erase_reset: %s\n", (true == status->erase_reset) ? "true" : "false" );
        printf( "status->illegal_command: %s\n", (true == status->illegal_command) ? "true" : "false" );
        printf( "status->com_crc_error: %s\n", (true == status->com_crc_error) ? "true" : "false" );
        printf( "status->erase_sequence_error: %s\n", (true == status->erase_sequence_error) ? "true" : "false" );
        printf( "status->address_error: %s\n", (true == status->address_error) ? "true" : "false" );
        printf( "status->parameter_error: %s\n", (true == status->parameter_error) ? "true" : "false" );
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
