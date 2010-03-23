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

#include "../src/ibus-radio-protocol.h"
#include <ibus-physical/ibus-physical.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MAX_PHYSICAL_MSG            10
#define TEST_IBUS_MAX_MESSAGE_SIZE  50

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static int32_t __physical_msg_count = 0;
static uint8_t __last_msg[TEST_IBUS_MAX_MESSAGE_SIZE];

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void test_irp_get_message( void );
void test_irp_send_normal_status( void );
void test_irp_disc_checking( void );
void test_irp_going_to_check_disc( void );
void test_strings( void );
static void check_last_msg( uint8_t *buffer, size_t size );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        suite = CU_add_suite( "ibus-radio-protocol Test", NULL, NULL );

        if( NULL != suite ) {
            CU_add_test( suite, "Test irp_init()                ", irp_init );
            CU_add_test( suite, "Test irp_get_message()         ", test_irp_get_message );
            CU_add_test( suite, "Test irp_send_announce()       ", irp_send_announce );
            CU_add_test( suite, "Test irp_send_poll_response()  ", irp_send_poll_response );
            CU_add_test( suite, "Test irp_send_normal_status()  ", test_irp_send_normal_status );
            CU_add_test( suite, "Test irp_completed_disc_check()", test_irp_disc_checking );
            CU_add_test( suite, "Test irp_going_to_check_disc() ", test_irp_going_to_check_disc );
            CU_add_test( suite, "Test strings                   ", test_strings );
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
void test_irp_get_message( void )
{
    irp_rx_msg_t *msg;

    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER, irp_get_message(NULL) );
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK, irp_get_message(msg) );
}

void test_irp_send_normal_status( void )
{
    int i;
    uint8_t no_magazine[]   = { 0x18, 0x0a, 0x68, 0x39, 0x0a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t no_discs[]      = { 0x18, 0x0a, 0x68, 0x39, 0x00, 0x02, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t stopped[]       = { 0x18, 0x0a, 0x68, 0x39, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t paused[]        = { 0x18, 0x0a, 0x68, 0x39, 0x01, 0x0c, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t playing_nrm[]   = { 0x18, 0x0a, 0x68, 0x39, 0x02, 0x09, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t playing_scn[]   = { 0x18, 0x0a, 0x68, 0x39, 0x02, 0x19, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t playing_rnd[]   = { 0x18, 0x0a, 0x68, 0x39, 0x02, 0x29, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t ff[]            = { 0x18, 0x0a, 0x68, 0x39, 0x03, 0x09, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t rew[]           = { 0x18, 0x0a, 0x68, 0x39, 0x04, 0x09, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t seeking[]       = { 0x18, 0x0a, 0x68, 0x39, 0x07, 0x09, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t seeking_next[]  = { 0x18, 0x0a, 0x68, 0x39, 0x05, 0x09, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t seeking_prev[]  = { 0x18, 0x0a, 0x68, 0x39, 0x06, 0x09, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00 };
    uint8_t loading_disc1[] = { 0x18, 0x0a, 0x68, 0x39, 0x08, 0x09, 0x00, 0x0f, 0x00, 0x01, 0x00, 0x00 };
    uint8_t loading_disc2[] = { 0x18, 0x0a, 0x68, 0x39, 0x08, 0x09, 0x00, 0x0f, 0x00, 0x02, 0x00, 0x00 };

    /* No magazine */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__STOPPED,
                                            IRP_MODE__NORMAL,
                                            false, 0, 0, 0) );
    check_last_msg( no_magazine, sizeof(no_magazine) );

    /* No discs */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__STOPPED,
                                            IRP_MODE__NORMAL,
                                            true, 0, 0, 0) );
    check_last_msg( no_discs, sizeof(no_discs) );

    /* Invalid input combos */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_send_normal_status(IRP_STATE__STOPPED,
                                            IRP_MODE__NORMAL,
                                            true, 1, 0, 0) );
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_send_normal_status(IRP_STATE__STOPPED,
                                            IRP_MODE__NORMAL,
                                            true, 1, 0, 1) );
    for( i = 2; i < 8; i++ ) {
        CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                         irp_send_normal_status(IRP_STATE__STOPPED,
                                                IRP_MODE__NORMAL,
                                                true, 1, (uint8_t) i, 1) );
    }
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_send_normal_status((irp_state_t) 0x99,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_send_normal_status(IRP_STATE__LOADING_DISC,
                                            IRP_MODE__NORMAL,
                                            true, 0x0f, 1, 1) );

    /* Stopped */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__STOPPED,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( stopped, sizeof(stopped) );

    /* Paused */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__PAUSED,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( paused, sizeof(paused) );

    /* Playing - Normal */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__PLAYING,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( playing_nrm, sizeof(playing_nrm) );

    /* Playing - Scanning */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__PLAYING,
                                            IRP_MODE__SCANNING,
                                            true, 1, 1, 1) );
    check_last_msg( playing_scn, sizeof(playing_scn) );

    /* Playing - Random */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__PLAYING,
                                            IRP_MODE__RANDOM,
                                            true, 1, 1, 1) );
    check_last_msg( playing_rnd, sizeof(playing_rnd) );

    /* FF */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__FAST_PLAYING__FORWARD,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( ff, sizeof(ff) );

    /* REW */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__FAST_PLAYING__REVERSE,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( rew, sizeof(rew) );

    /* Seeking */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__SEEKING,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( seeking, sizeof(seeking) );

    /* Seeking - Next */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__SEEKING__NEXT,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( seeking_next, sizeof(seeking_next) );

    /* Seeking - Prev */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__SEEKING__PREV,
                                            IRP_MODE__NORMAL,
                                            true, 1, 1, 1) );
    check_last_msg( seeking_prev, sizeof(seeking_prev) );

    /* Loading Disc */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__LOADING_DISC,
                                            IRP_MODE__NORMAL,
                                            true, 0x0f, 1, 0) );
    check_last_msg( loading_disc1, sizeof(loading_disc1) );

    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_send_normal_status(IRP_STATE__LOADING_DISC,
                                            IRP_MODE__NORMAL,
                                            true, 0x0f, 2, 0) );
    check_last_msg( loading_disc2, sizeof(loading_disc2) );
}

void test_irp_disc_checking( void )
{
    uint8_t valid_disc_stop[] = { 0x18, 0x0a, 0x68, 0x39, 0x09, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00 };
    uint8_t valid_disc_play[] = { 0x18, 0x0a, 0x68, 0x39, 0x09, 0x09, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00 };
    uint8_t no_disc_stop[]    = { 0x18, 0x0a, 0x68, 0x39, 0x09, 0x02, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00 };
    uint8_t no_disc_play[]    = { 0x18, 0x0a, 0x68, 0x39, 0x09, 0x09, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00 };

    /* Disc number is too low. */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_completed_disc_check(0, false, 0, IRP_STATE__PLAYING) );

    /* Disc number is too high. */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_completed_disc_check(7, false, 0, IRP_STATE__PLAYING) );

    /* Disc bitmap is defined for a disc greater than the current value. */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_completed_disc_check(1, false, 0xf0, IRP_STATE__PLAYING) );

    /* Disc bitmap mismatch */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_completed_disc_check(1, true, 0x00, IRP_STATE__PLAYING) );
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_completed_disc_check(1, false, 0x01, IRP_STATE__PLAYING) );

    /* Invalid goal */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_completed_disc_check(1, true, 0x01, IRP_STATE__PAUSED) );

    /* Valid cases */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_completed_disc_check(1, true, 0x01, IRP_STATE__STOPPED) );
    check_last_msg( valid_disc_stop, sizeof(valid_disc_stop) );

    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_completed_disc_check(1, false, 0x00, IRP_STATE__STOPPED) );
    check_last_msg( no_disc_stop, sizeof(no_disc_stop) );

    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_completed_disc_check(1, true, 0x01, IRP_STATE__PLAYING) );
    check_last_msg( valid_disc_play, sizeof(valid_disc_play) );

    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_completed_disc_check(1, false, 0x00, IRP_STATE__PLAYING) );
    check_last_msg( no_disc_play, sizeof(no_disc_play) );
}

void test_irp_going_to_check_disc( void )
{
    uint8_t start_checking_play[] = { 0x18, 0x0a, 0x68, 0x39, 0x09, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 };
    uint8_t start_checking_stop[] = { 0x18, 0x0a, 0x68, 0x39, 0x09, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 };

    /* Disc number is too low. */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_going_to_check_disc(0, 0, IRP_STATE__PLAYING) );

    /* Disc number is too high. */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_going_to_check_disc(7, 0, IRP_STATE__PLAYING) );

    /* Disc bitmap is defined for a disc greater than the current value. */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_going_to_check_disc(1, 0xf0, IRP_STATE__PLAYING) );

    /* Invalid goal */
    CU_ASSERT_EQUAL_FATAL( IRP_ERROR_PARAMETER,
                     irp_going_to_check_disc(1, 0x01, IRP_STATE__PAUSED) );

    /* Going to check discs */
    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_going_to_check_disc(1, 0x00, IRP_STATE__PLAYING) );
    check_last_msg( start_checking_play, sizeof(start_checking_play) );

    CU_ASSERT_EQUAL_FATAL( IRP_RETURN_OK,
                     irp_going_to_check_disc(1, 0x00, IRP_STATE__STOPPED) );
    check_last_msg( start_checking_stop, sizeof(start_checking_stop) );
}

void test_strings( void )
{
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__STOPPED), "IRP_STATE__STOPPED" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__PAUSED), "IRP_STATE__PAUSED" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__PLAYING), "IRP_STATE__PLAYING" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__FAST_PLAYING__FORWARD), "IRP_STATE__FAST_PLAYING__FORWARD" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__FAST_PLAYING__REVERSE), "IRP_STATE__FAST_PLAYING__REVERSE" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__SEEKING), "IRP_STATE__SEEKING" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__SEEKING__NEXT), "IRP_STATE__SEEKING__NEXT" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__SEEKING__PREV), "IRP_STATE__SEEKING__PREV" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string(IRP_STATE__LOADING_DISC), "IRP_STATE__LOADING_DISC" );
    CU_ASSERT_STRING_EQUAL( irp_state_to_string((irp_state_t) 0x999), "unknown" );

    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__GET_STATUS), "IRP_CMD__GET_STATUS" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__STOP), "IRP_CMD__STOP" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__PAUSE), "IRP_CMD__PAUSE" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__PLAY), "IRP_CMD__PLAY" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__FAST_PLAY__FORWARD), "IRP_CMD__FAST_PLAY__FORWARD" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__FAST_PLAY__REVERSE), "IRP_CMD__FAST_PLAY__REVERSE" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__SEEK__NEXT), "IRP_CMD__SEEK__NEXT" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__SEEK__PREV), "IRP_CMD__SEEK__PREV" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__SEEK__ALT_NEXT), "IRP_CMD__SEEK__ALT_NEXT" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__SEEK__ALT_PREV), "IRP_CMD__SEEK__ALT_PREV" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__SCAN_DISC__ENABLE), "IRP_CMD__SCAN_DISC__ENABLE" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__SCAN_DISC__DISABLE), "IRP_CMD__SCAN_DISC__DISABLE" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__RANDOMIZE__ENABLE), "IRP_CMD__RANDOMIZE__ENABLE" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__RANDOMIZE__DISABLE), "IRP_CMD__RANDOMIZE__DISABLE" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__CHANGE_DISC), "IRP_CMD__CHANGE_DISC" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__POLL), "IRP_CMD__POLL" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string(IRP_CMD__TRAFFIC), "IRP_CMD__TRAFFIC" );
    CU_ASSERT_STRING_EQUAL( irp_cmd_to_string((irp_cmd_t) 0x999), "unknown" );

    CU_ASSERT_STRING_EQUAL( irp_mode_to_string(IRP_MODE__NORMAL), "IRP_MODE__NORMAL" );
    CU_ASSERT_STRING_EQUAL( irp_mode_to_string(IRP_MODE__SCANNING), "IRP_MODE__SCANNING" );
    CU_ASSERT_STRING_EQUAL( irp_mode_to_string(IRP_MODE__RANDOM), "IRP_MODE__RANDOM" );
    CU_ASSERT_STRING_EQUAL( irp_mode_to_string((irp_mode_t) 0x999), "unknown" );
}

/*----------------------------------------------------------------------------*/
/*                               Mock functions                               */
/*----------------------------------------------------------------------------*/
void ibus_physical_init( void )
{
}

void ibus_physical_destroy( void )
{
}

ibus_io_msg_t* ibus_physical_get_message( void )
{
    CU_ASSERT_EQUAL_FATAL( 0, __physical_msg_count );
    __physical_msg_count++;
    return NULL;
}

void ibus_physical_release_message( ibus_io_msg_t* msg )
{
    CU_ASSERT_EQUAL_FATAL( 1, __physical_msg_count );
    __physical_msg_count--;
}

bool message_converter( const ibus_io_msg_t *in, irp_rx_msg_t *out )
{
    static int count = 0;
    CU_ASSERT_EQUAL_FATAL( NULL, in );

    count++;
    if( count < 10 ) {
        return false;
    }

    if( 20 == count ) {
        count = 0;
    }

    return true;
}

bool ibus_physical_send_message( const uint8_t *msg, const size_t size )
{
    memcpy( __last_msg, msg, size );
    return true;
}

static void check_last_msg( uint8_t *buffer, size_t size )
{
    int i;
    uint8_t checksum;

    checksum = 0;
    for( i = 0; i < size - 1; i++ ) {
        checksum ^= buffer[i];
    }
    buffer[i] = checksum;

    for( i = 0; i < size; i++ ) {
        if( buffer[i] != __last_msg[i] ) {
            printf( "%d 0x%02x =?= 0x%02x\n", i, buffer[i], __last_msg[i] );
        }
        CU_ASSERT_EQUAL_FATAL( buffer[i], __last_msg[i] );
    }

    memset( __last_msg, 0, TEST_IBUS_MAX_MESSAGE_SIZE );
}
