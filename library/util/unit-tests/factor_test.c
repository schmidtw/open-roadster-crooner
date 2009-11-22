#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "../src/factor.h"

void test_factor( void )
{
    uint32_t pow, rem;

    factor_out_two( 0, &pow, &rem );
    CU_ASSERT_EQUAL( 0, pow );
    CU_ASSERT_EQUAL( 0, rem );

    factor_out_two( 1, &pow, &rem );
    CU_ASSERT_EQUAL( 0, pow );
    CU_ASSERT_EQUAL( 1, rem );

    factor_out_two( 2, &pow, &rem );
    CU_ASSERT_EQUAL( 1, pow );
    CU_ASSERT_EQUAL( 1, rem );

    factor_out_two( 1024, &pow, &rem );
    CU_ASSERT_EQUAL( 10, pow );
    CU_ASSERT_EQUAL( 1, rem );

    factor_out_two( 3072, &pow, &rem );
    CU_ASSERT_EQUAL( 10, pow );
    CU_ASSERT_EQUAL( 3, rem );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Factor Test", NULL, NULL );
    CU_add_test( *suite, "Test factor()", test_factor );
}

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
