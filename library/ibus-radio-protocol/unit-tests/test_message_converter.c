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
#include <stdbool.h>
#include <stdint.h>

#include <ibus-physical/ibus-physical.h>

#include "message-converter.h"

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
static void test( void );
static void calc_checksum( ibus_io_msg_t *in );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        suite = CU_add_suite( "ibus-radio-protocol Test", NULL, NULL );

        if( NULL != suite ) {
            CU_add_test( suite, "Test message_converter()", test );
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
static void test( void )
{
    ibus_io_msg_t in;
    irp_rx_msg_t out;

    out.command = IRP_CMD__POLL;

    /* Input checking */
    CU_ASSERT_EQUAL( false,
                     message_converter(NULL, NULL) );
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, NULL) );

    memset( &in, 0, sizeof(in) );

    /* Error state */
    in.status = IBUS_IO_STATUS__BUFFER_OVERRUN_ERROR;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Length is too short. */
    in.status = IBUS_IO_STATUS__OK;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Bad CRC */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 5;
    in.buffer[0] = 0x68;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* We sent the message. */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 5;
    in.buffer[0] = IBUS_DEVICE__CD_CHANGER;
    calc_checksum( &in );
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Message not to us */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 5;
    in.buffer[0] = IBUS_DEVICE__PHONE;
    in.buffer[1] = 3;
    in.buffer[2] = IBUS_DEVICE__IKE;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__TRAFFIC, out.command );

    /* Message is poll request */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 5;
    in.buffer[0] = IBUS_DEVICE__RADIO;
    in.buffer[1] = 3;
    in.buffer[2] = IBUS_DEVICE__BROADCAST_LOW;
    in.buffer[3] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__TRAFFIC;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    in.buffer[2] = IBUS_DEVICE__BROADCAST_HIGH;
    calc_checksum( &in );
    out.command = IRP_CMD__TRAFFIC;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    in.buffer[2] = IBUS_DEVICE__CD_CHANGER;
    calc_checksum( &in );
    out.command = IRP_CMD__TRAFFIC;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Unknown message of length 3 */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 5;
    in.buffer[0] = IBUS_DEVICE__RADIO;
    in.buffer[1] = 3;
    in.buffer[2] = IBUS_DEVICE__BROADCAST_LOW;
    in.buffer[3] = 0x09;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Unknown message of length 5 */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 7;
    in.buffer[0] = IBUS_DEVICE__RADIO;
    in.buffer[1] = 5;
    in.buffer[2] = IBUS_DEVICE__BROADCAST_LOW;
    in.buffer[3] = 0x09;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Normal messages */

    /* Status Request */
    in.status = IBUS_IO_STATUS__OK;
    in.size = 7;
    in.buffer[0] = IBUS_DEVICE__RADIO;
    in.buffer[1] = 5;
    in.buffer[2] = IBUS_DEVICE__CD_CHANGER;
    in.buffer[3] = 0x38;
    in.buffer[4] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__GET_STATUS, out.command );

    /* Stop */
    in.buffer[4] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__STOP, out.command );

    /* Pause */
    in.buffer[4] = 0x02;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__PAUSE, out.command );

    /* Play */
    in.buffer[4] = 0x03;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__PLAY, out.command );

    /* Fast Play - Rewind */
    in.buffer[4] = 0x04;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__FAST_PLAY__REVERSE, out.command );

    /* Fast Play - Forward */
    in.buffer[4] = 0x04;
    in.buffer[5] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__FAST_PLAY__FORWARD, out.command );

    /* Fast Play - Invalid */
    in.buffer[4] = 0x04;
    in.buffer[5] = 0x03;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Seek - Next */
    in.buffer[4] = 0x05;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__SEEK__NEXT, out.command );

    /* Seek - Prev */
    in.buffer[4] = 0x05;
    in.buffer[5] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__SEEK__PREV, out.command );

    /* Seek - Invalid */
    in.buffer[4] = 0x05;
    in.buffer[5] = 0x03;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Alt Seek - Next */
    in.buffer[4] = 0x0a;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__SEEK__ALT_NEXT, out.command );

    /* Alt Seek - Prev */
    in.buffer[4] = 0x0a;
    in.buffer[5] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__SEEK__ALT_PREV, out.command );

    /* Alt Seek - Invalid */
    in.buffer[4] = 0x0a;
    in.buffer[5] = 0x03;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Change Disc - Valid */
    in.buffer[4] = 0x06;
    in.buffer[5] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__CHANGE_DISC, out.command );

    /* Change Disc - Invalid - Low */
    in.buffer[4] = 0x06;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Change Disc - Invalid - High */
    in.buffer[4] = 0x06;
    in.buffer[5] = 0x09;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Scan Disc - On */
    in.buffer[4] = 0x07;
    in.buffer[5] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__SCAN_DISC__ENABLE, out.command );

    /* Scan Disc - Off */
    in.buffer[4] = 0x07;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__SCAN_DISC__DISABLE, out.command );

    /* Scan Disc - Invalid */
    in.buffer[4] = 0x07;
    in.buffer[5] = 0x03;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Random - On */
    in.buffer[4] = 0x08;
    in.buffer[5] = 0x01;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__RANDOMIZE__ENABLE, out.command );

    /* Random - Off */
    in.buffer[4] = 0x08;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( true,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__RANDOMIZE__DISABLE, out.command );

    /* Random - Invalid */
    in.buffer[4] = 0x08;
    in.buffer[5] = 0x03;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

    /* Invalid Command*/
    in.buffer[4] = 0x19;
    in.buffer[5] = 0x00;
    calc_checksum( &in );
    out.command = IRP_CMD__POLL;
    CU_ASSERT_EQUAL( false,
                     message_converter(&in, &out) );
    CU_ASSERT_EQUAL( IRP_CMD__POLL, out.command );

}

static void calc_checksum( ibus_io_msg_t *in )
{
    int i;
    uint8_t checksum;

    checksum = 0;
    for( i = 0; i < (in->size - 1); i++ ) {
        checksum ^= in->buffer[i];
    }

    in->buffer[i] = checksum;
}
