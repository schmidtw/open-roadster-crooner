/*
 * Copyright (c) 2012  Weston Schmidt
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

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <CUnit/Basic.h>

#include <freertos/os-mock.h>

#include "../src/led.h"

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
extern void mock_os_init_std( void );
extern void mock_os_destroy_std( void );

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static int __fake_free_called = 0;
void fake_free( void *p )
{
    __fake_free_called++;
}

void test_basic( void )
{
    led_state_t green = { .red = 0, .green = 255, .blue = 0, .duration = 0 };
    led_state_t red_flash[] = {
        { .red = 255, .green = 0, .blue = 0, .duration = 10 },
        { .red =   0, .green = 0, .blue = 0, .duration = 10 } };

    MOCK_reset__os();
    __fake_free_called = 0;

    CU_ASSERT( LED_RETURN_OK == led_init(0) );

    led_set_state( red_flash, 2, true, fake_free );
    os_task_delay_ms( 1000 );
    led_set_state( &green, 1, false, NULL );
    led_set_state( red_flash, 2, false, NULL );
    os_task_delay_ms( 1000 );

    led_destroy();

    CU_ASSERT( 1 == __fake_free_called );
}

void test_stress( void )
{
    led_state_t green = { .red = 0, .green = 255, .blue = 0, .duration = 0 };

    MOCK_reset__os();

    CU_ASSERT( LED_RETURN_OK == led_init(0) );

    led_set_state( &green, 1, false, NULL );
    led_set_state( &green, 1, false, NULL );
    led_set_state( &green, 1, false, NULL );
    led_set_state( &green, 1, false, NULL );

    led_destroy();
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "LED Test", NULL, NULL );
    CU_add_test( *suite, "Test Basic System", test_basic );
}

int main( int argc, char *argv[] )
{
    CU_pSuite suite = NULL;

    MOCK_os_init();

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

    MOCK_os_destroy();

    return CU_get_error();
}
