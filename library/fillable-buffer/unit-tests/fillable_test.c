#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "../src/fillable-buffer.h"

static uint32_t my_data = 4;

static void *__my_flush_data = NULL;
static uint32_t __my_flush_i = 0;
static uint8_t *__my_flush_buf = NULL;
static uint32_t __my_flush_size = 0;
void my_flush( void *data, const uint8_t *buf, const size_t size )
{
    uint32_t i;

    CU_ASSERT_EQUAL( __my_flush_data, data );
    CU_ASSERT_EQUAL( __my_flush_size, size );

    for( i = 0; i < size; i++ ) {
        CU_ASSERT_EQUAL( __my_flush_buf[__my_flush_i], buf[i] );
        __my_flush_i++;
    }
}

void test_fillbuf_append( void )
{
    int i;
    int rv;
    uint8_t buffer[24];
    const uint8_t good8[] = { 0x90, 0x91, 0x92, 0x93,
                              0x94, 0x95, 0x96, 0x97 };
    fillable_buffer_t fb;

    memset( buffer, 0x55, sizeof(buffer) );

    fb.buf = &buffer[8];
    fb.size = 8;
    fb.offset = 0;
    fb.data = &my_data;
    fb.flush = my_flush;

    __my_flush_data = &my_data;
    __my_flush_i = 0;
    __my_flush_buf = (uint8_t *) good8;
    __my_flush_size = 8;
    rv = fillbuf_append( &fb, good8, 8 );

    CU_ASSERT_EQUAL( 0, rv );
    for( i = 0; i < 8; i++ ) {
        CU_ASSERT_EQUAL( buffer[i], 0x55 );
    }
    for( i = 16; i < 24; i++ ) {
        CU_ASSERT_EQUAL( buffer[i], 0x55 );
    }

    /* null callback function */
    fb.buf = &buffer[8];
    fb.size = 8;
    fb.offset = 0;
    fb.data = &my_data;
    fb.flush = NULL;
    __my_flush_size = 0;


    /* send more than one block of data */
    memset( buffer, 0x55, sizeof(buffer) );
    fb.buf = &buffer[8];
    fb.size = 3;
    fb.offset = 0;
    fb.data = &my_data;
    fb.flush = my_flush;

    __my_flush_data = &my_data;
    __my_flush_i = 0;
    __my_flush_buf = (uint8_t *) good8;
    __my_flush_size = 3;
    rv = fillbuf_append( &fb, good8, 8 );

    CU_ASSERT_EQUAL( 0, rv );
    for( i = 0; i < 8; i++ ) {
        CU_ASSERT_EQUAL( buffer[i], 0x55 );
    }
    for( i = 11; i < 24; i++ ) {
        CU_ASSERT_EQUAL( buffer[i], 0x55 );
    }

    /* input boundary tests */
    fb.buf = NULL;
    fb.size = 3;
    fb.offset = 0;
    fb.data = &my_data;
    fb.flush = NULL;

    rv = fillbuf_append( &fb, good8, 8 );
    CU_ASSERT( rv < 0 );

    rv = fillbuf_append( NULL, good8, 8 );
    CU_ASSERT( rv < 0 );
}

void test_fillbuf_flush( void )
{
    int rv;
    fillable_buffer_t fb;

    /* input boundary tests (all that's needed) */
    fb.buf = NULL;
    fb.size = 3;
    fb.offset = 0;
    fb.data = &my_data;
    fb.flush = NULL;

    rv = fillbuf_flush( &fb );
    CU_ASSERT( rv < 0 );

    rv = fillbuf_flush( NULL );
    CU_ASSERT( rv < 0 );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "fillable-buffer Test", NULL, NULL );
    CU_add_test( *suite, "Test fillbuf_append()", test_fillbuf_append );
    CU_add_test( *suite, "Test fillbuf_flush()", test_fillbuf_flush );
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
