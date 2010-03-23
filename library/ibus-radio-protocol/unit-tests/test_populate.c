/*
 * Copyright (c) 2010  Weston Schmidt
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

#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdint.h>

#include "populate-message.h"

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
void test( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        suite = CU_add_suite( "ibus-radio-protocol Test", NULL, NULL );

        if( NULL != suite ) {
            CU_add_test( suite, "Test populate_message()", test );
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
        }

        CU_cleanup_registry();
    }

    return CU_get_error();
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void check( uint8_t *payload, uint8_t *out )
{
    int i;

    for( i = 0; i < 300; i++ ) {
        if( 8 != i ) {
            CU_ASSERT_EQUAL( 0x55, payload[i] );
        } else {
            CU_ASSERT_EQUAL( 0x01, payload[i] );
        }
        CU_ASSERT_EQUAL( 0x55, out[i] );
    }
}

void test( void )
{
    int i;
    uint8_t payload[300];
    uint8_t out[300];

    memset( payload, 0x55, sizeof(payload) );
    memset( out, 0x55, sizeof(out) );

    payload[8] = 0x01;

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_ERROR_PARAMETER,
                     populate_message( IBUS_DEVICE__BROADCAST_LOW,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       NULL,
                                       0,
                                       NULL,
                                       0 ) );

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_ERROR_PARAMETER,
                     populate_message( IBUS_DEVICE__BROADCAST_LOW,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       &payload[8],
                                       0,
                                       NULL,
                                       0 ) );

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_ERROR_PARAMETER,
                     populate_message( IBUS_DEVICE__BROADCAST_LOW,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       &payload[8],
                                       250,
                                       NULL,
                                       0 ) );

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_ERROR_PARAMETER,
                     populate_message( IBUS_DEVICE__BROADCAST_LOW,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       &payload[8],
                                       255,
                                       NULL,
                                       0 ) );

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_ERROR_PARAMETER,
                     populate_message( IBUS_DEVICE__BROADCAST_LOW,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       &payload[8],
                                       1,
                                       NULL,
                                       0 ) );

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_ERROR_PARAMETER,
                     populate_message( IBUS_DEVICE__BROADCAST_LOW,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       &payload[8],
                                       1,
                                       &out[8],
                                       1 ) );

    check( payload, out );
    CU_ASSERT_EQUAL( IRP_RETURN_OK,
                     populate_message( IBUS_DEVICE__RADIO,
                                       IBUS_DEVICE__BROADCAST_LOW,
                                       &payload[8],
                                       1,
                                       &out[8],
                                       5 ) );

    for( i = 0; i < 300; i++ ) {
        if( 8 != i ) {
            CU_ASSERT_EQUAL( 0x55, payload[i] );
        } else {
            CU_ASSERT_EQUAL( 0x01, payload[i] );
        }
        if( (8 <= i) && (i <= 12) ) {
        } else {
            CU_ASSERT_EQUAL( 0x55, out[i] );
        }
    }
}
