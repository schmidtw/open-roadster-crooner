#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "../src/circular-buffer.h"
#include <stdint.h>

void test_invalid_create( void ) {
    void * cb;
    CU_ASSERT_EQUAL(cb_create_list(0, 5), NULL);
    CU_ASSERT_EQUAL(cb_create_list(5, 0), NULL);

    cb = cb_create_list(8, 5);
    CU_ASSERT_FALSE( cb_push(cb, NULL) );
    CU_ASSERT_FALSE( cb_push(NULL, cb) );

    cb_destroy_list(&cb);
    CU_ASSERT_EQUAL(cb, NULL);

    cb_clear_list(NULL);
}

void test_create_fill_use_case( void )
{
    typedef struct {
        uint32_t value;
        uint16_t key;
    } test_element_t;

    test_element_t local_element;
    void * cb;

    cb = cb_create_list(sizeof(test_element_t), 5);
    CU_ASSERT_NOT_EQUAL(cb, NULL);

    CU_ASSERT_EQUAL( cb_peek_tail(cb), NULL );

    local_element.value = 1;
    local_element.key = 500;

    cb_push(cb, &local_element);
    {
        test_element_t *t = (test_element_t*)cb_peek_tail(cb);
        CU_ASSERT_EQUAL(t->value, 1);
        CU_ASSERT_EQUAL(t->key, 500);
    }
    local_element.value = 2;
    CU_ASSERT_TRUE(cb_push(cb, &local_element));
    local_element.value = 3;
    CU_ASSERT_TRUE(cb_push(cb, &local_element));
    local_element.value = 4;
    CU_ASSERT_TRUE(cb_push(cb, &local_element));
    local_element.value = 5;
    CU_ASSERT_TRUE(cb_push(cb, &local_element));

    CU_ASSERT_TRUE(cb_pop(cb, &local_element));
    CU_ASSERT_EQUAL(local_element.value, 5);

    CU_ASSERT_TRUE(cb_pop(cb, &local_element));
    CU_ASSERT_EQUAL(local_element.value, 4);

    CU_ASSERT_TRUE(cb_pop(cb, &local_element));
    CU_ASSERT_EQUAL(local_element.value, 3);

    CU_ASSERT_TRUE(cb_pop(cb, &local_element));
    CU_ASSERT_EQUAL(local_element.value, 2);

    CU_ASSERT_TRUE(cb_pop(cb, &local_element));
    CU_ASSERT_EQUAL(local_element.value, 1);

    CU_ASSERT_FALSE(cb_pop(cb, &local_element));

    {
        int i;
        for( i = 1; i <= 7; i++ ) {
            local_element.value = i;
            CU_ASSERT_TRUE(cb_push(cb, &local_element));
        }
        for( i = 7; i > 2; i-- ) {
            CU_ASSERT_TRUE(cb_pop(cb, &local_element));
            CU_ASSERT_EQUAL(local_element.value, i);
        }
        CU_ASSERT_FALSE(cb_pop(cb, &local_element));
    }

    cb_destroy_list(&cb);
    CU_ASSERT_EQUAL(cb,NULL);

}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Singly Linked List Test", NULL, NULL );
    CU_add_test( *suite, "Test invalid params()              ", test_invalid_create );
    CU_add_test( *suite, "Test filling circular buffer()     ", test_create_fill_use_case );
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
