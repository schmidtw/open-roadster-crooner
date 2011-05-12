#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include "file_helper.h"

void test_get_dir( void )
{
    char src[200];
    char dst[200];

    CU_ASSERT(false == get_last_dir_name(dst, NULL));
    CU_ASSERT(false == get_last_dir_name(NULL, src));
    strcpy(src, "green.txt");
    CU_ASSERT(false == get_last_dir_name(dst, src));

    strcpy(src, "/blue/black/green.txt");
    CU_ASSERT(true == get_last_dir_name(dst, src));
    CU_ASSERT(0 == strcmp(src, "/blue/black"));
    CU_ASSERT(0 == strcmp(dst, "green.txt"));

    CU_ASSERT(true == get_last_dir_name(dst, src));
    CU_ASSERT(0 == strcmp(src, "/blue"));
    CU_ASSERT(0 == strcmp(dst, "black"));

    CU_ASSERT(true == get_last_dir_name(dst, src));
    CU_ASSERT(0 == strcmp(src, "/"));
    CU_ASSERT(0 == strcmp(dst, "blue"));

    CU_ASSERT(false == get_last_dir_name(dst, src));
}

void test_append( void )
{
    char dst[200];

    append_to_path(NULL, "/");
    strcpy(dst, "/");
    append_to_path(dst, NULL);
    CU_ASSERT(0 == strcmp(dst, "/"));


    strcpy(dst, "/");
    append_to_path(dst, "blue");
    CU_ASSERT(0 == strcmp(dst, "/blue"));

    strcpy(dst, "");
    append_to_path(dst, "blue");
    CU_ASSERT(0 == strcmp(dst, "/blue"));

    append_to_path(dst, "green");
    CU_ASSERT(0 == strcmp(dst, "/blue/green"));

}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "File Helper Test", NULL, NULL );
    CU_add_test( *suite, "Test Getting the Last Directory Name   ", test_get_dir );
    CU_add_test( *suite, "Test Appending to the Path   ", test_append );
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
