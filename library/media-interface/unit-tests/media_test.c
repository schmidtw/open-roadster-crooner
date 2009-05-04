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

#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <linked-list/linked-list.h>

#include "../src/media-interface.h"

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
static void add_suites( CU_pSuite *suite );
static void test_registration( void );
static void test_get_information( void );
static media_status_t command( const media_command_t cmd );
static media_status_t play( const char *filename, media_suspend_fn_t suspend, media_resume_fn_t resume );
static bool get_type_true( const char *filename );
static bool get_type_false( const char *filename );
static media_status_t metadata_ok( const char *filename, media_metadata_t *metadata );
static media_status_t metadata_fail( const char *filename, media_metadata_t *metadata );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
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
static void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Media Interface Test", NULL, NULL );
    CU_add_test( *suite, "Registration Test", test_registration );
    CU_add_test( *suite, "Get Information Test", test_get_information );
}

static void test_registration( void )
{
    media_interface_t *mi;
    ll_list_t *l;

    mi = media_new();
    CU_ASSERT( NULL != mi );
    CU_ASSERT( MI_ERROR_PARAMETER == media_delete(NULL) );
    CU_ASSERT( MI_RETURN_OK == media_delete(mi) );

    CU_ASSERT( MI_ERROR_PARAMETER == media_register_codec(NULL, NULL, NULL, NULL, NULL, NULL) );

    mi = media_new();
    CU_ASSERT( NULL != mi );
    l = (ll_list_t *) mi;
    CU_ASSERT( MI_ERROR_PARAMETER == media_register_codec(mi, NULL, NULL, NULL, NULL, NULL) );
    CU_ASSERT( NULL == l->head );
    CU_ASSERT( NULL == l->tail );
    CU_ASSERT( MI_ERROR_PARAMETER == media_register_codec(mi, "Foo", NULL, NULL, NULL, NULL) );
    CU_ASSERT( NULL == l->head );
    CU_ASSERT( NULL == l->tail );
    CU_ASSERT( MI_ERROR_PARAMETER == media_register_codec(mi, "Foo", &command, NULL, NULL, NULL) );
    CU_ASSERT( NULL == l->head );
    CU_ASSERT( NULL == l->tail );
    CU_ASSERT( MI_ERROR_PARAMETER == media_register_codec(mi, "Foo", &command, &play, NULL, NULL) );
    CU_ASSERT( NULL == l->head );
    CU_ASSERT( NULL == l->tail );
    CU_ASSERT( MI_ERROR_PARAMETER == media_register_codec(mi, "Foo", &command, &play, &get_type_false, NULL) );
    CU_ASSERT( NULL == l->head );
    CU_ASSERT( NULL == l->tail );
    CU_ASSERT( MI_RETURN_OK == media_register_codec(mi, "Foo", &command, &play, &get_type_false, &metadata_ok) );
    CU_ASSERT( NULL != l->head );
    CU_ASSERT( NULL != l->tail );
    CU_ASSERT( l->head == l->tail );
    CU_ASSERT( MI_RETURN_OK == media_register_codec(mi, "Bar", &command, &play, &get_type_false, &metadata_ok) );
    CU_ASSERT( NULL != l->head );
    CU_ASSERT( NULL != l->tail );
    CU_ASSERT( l->head->next == l->tail );
    CU_ASSERT( l->head == l->tail->prev );
    CU_ASSERT( MI_RETURN_OK == media_delete(mi) );
}

static void test_get_information( void )
{
    media_interface_t *mi;
    media_metadata_t data;
    media_command_fn_t command_fn;
    media_play_fn_t play_fn;

    CU_ASSERT( MI_ERROR_PARAMETER == media_get_information(NULL, NULL, NULL, NULL, NULL) );
    mi = media_new();
    CU_ASSERT( NULL != mi );
    CU_ASSERT( MI_ERROR_PARAMETER == media_get_information(mi, NULL, NULL, NULL, NULL) );
    CU_ASSERT( MI_ERROR_PARAMETER == media_get_information(mi, "FileName", NULL, NULL, NULL) );
    CU_ASSERT( MI_ERROR_NOT_SUPPORTED == media_get_information(mi, "FileName", &data, NULL, NULL) );
    CU_ASSERT( MI_RETURN_OK == media_register_codec(mi, "Foo", &command, &play, &get_type_false, &metadata_ok) );
    CU_ASSERT( MI_ERROR_NOT_SUPPORTED == media_get_information(mi, "FileName", &data, NULL, NULL) );
    CU_ASSERT( MI_RETURN_OK == media_register_codec(mi, "Bar", &command, &play, &get_type_true, &metadata_ok) );
    CU_ASSERT( MI_RETURN_OK == media_get_information(mi, "FileName", &data, NULL, NULL) );
    CU_ASSERT( MI_RETURN_OK == media_get_information(mi, "FileName", NULL, &command_fn, NULL) );
    CU_ASSERT( command_fn == command );
    CU_ASSERT( MI_RETURN_OK == media_get_information(mi, "FileName", NULL, NULL, &play_fn) );
    CU_ASSERT( play_fn == play );

    CU_ASSERT( MI_RETURN_OK == media_delete(mi) );

    mi = media_new();
    CU_ASSERT( NULL != mi );
    CU_ASSERT( MI_RETURN_OK == media_register_codec(mi, "Bar", &command, &play, &get_type_true, &metadata_fail) );
    CU_ASSERT( MI_ERROR_DECODE_ERROR == media_get_information(mi, "FileName", &data, NULL, NULL) );
    CU_ASSERT( MI_RETURN_OK == media_delete(mi) );
}

static media_status_t command( const media_command_t cmd )
{
    return MI_RETURN_OK;
}

static media_status_t play( const char *filename, media_suspend_fn_t suspend, media_resume_fn_t resume )
{
    return MI_RETURN_OK;
}

static bool get_type_true( const char *filename )
{
    CU_ASSERT( NULL != filename );
    return true;
}

static bool get_type_false( const char *filename )
{
    CU_ASSERT( NULL != filename );
    return false;
}

static media_status_t metadata_ok( const char *filename, media_metadata_t *metadata )
{
    CU_ASSERT( NULL != filename );
    CU_ASSERT( NULL != metadata );

    return MI_RETURN_OK;
}

static media_status_t metadata_fail( const char *filename, media_metadata_t *metadata )
{
    CU_ASSERT( NULL != filename );
    CU_ASSERT( NULL != metadata );

    return MI_ERROR_DECODE_ERROR;
}
