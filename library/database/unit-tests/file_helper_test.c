#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include "file_helper.h"

void test_remove_dir( void )
{
    char string[200];
    size_t string_len = 0;

    CU_ASSERT(false == remove_last_dir_name(string, NULL));
    CU_ASSERT(false == remove_last_dir_name(NULL, &string_len));

    strcpy(string, "green.txt");
    string_len = strlen(string);
    CU_ASSERT(false == remove_last_dir_name(string, &string_len));

    strcpy(string, "/blue/black/green.txt");
    string_len = strlen(string);
    CU_ASSERT(true == remove_last_dir_name(string, &string_len));
    CU_ASSERT( 0 == strcmp(string, "/blue/black") );
    CU_ASSERT( string_len == 11 );

    CU_ASSERT(true == remove_last_dir_name(string, &string_len));
    CU_ASSERT(0 == strcmp(string, "/blue"));
    CU_ASSERT( string_len == 5 );

    CU_ASSERT(true == remove_last_dir_name(string, &string_len));
    CU_ASSERT(0 == strcmp(string, "/"));
    CU_ASSERT( string_len == 1 );

    CU_ASSERT(false == remove_last_dir_name(string, &string_len));

    strcpy(string, "/blue/black/");
    string_len = strlen(string);
    CU_ASSERT(true == remove_last_dir_name(string, &string_len));
    CU_ASSERT(0 == strcmp(string, "/blue"));
    CU_ASSERT( string_len == 5 );
}

void test_append( void )
{
    char dst[200];
    size_t len = 0;

    bzero(dst, sizeof(dst));

    CU_ASSERT( false == append_to_path(NULL, &len, "/") );
    CU_ASSERT( false == append_to_path(dst, NULL, "/") );
    CU_ASSERT( false == append_to_path(dst, &len, NULL) );

    strcpy(dst, "/");
    len = strlen(dst);
    CU_ASSERT( true == append_to_path(dst, &len, "blue") );
    CU_ASSERT(0 == strcmp(dst, "/blue"));
    CU_ASSERT(5 == len);

    CU_ASSERT( true == append_to_path(dst, &len, "green") );
    CU_ASSERT(0 == strcmp(dst, "/blue/green"));
    CU_ASSERT(11 == len);

    bzero(dst, sizeof(dst));
    strcpy(dst, "/blue/");
    len = strlen(dst);
    CU_ASSERT( true == append_to_path(dst, &len, "green") );
    CU_ASSERT(0 == strcmp(dst, "/blue/green"));
    CU_ASSERT(11 == len);

}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "File Helper Test", NULL, NULL );
    CU_add_test( *suite, "Test Removing the Last Directory/File Name   ", test_remove_dir );
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
