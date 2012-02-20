#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "../src/binary-tree-avl.h"

//#define ENABLE_DEBUG 1

bt_ir_t iterate_print( bt_node_t *node, void *user_data )
{
#ifdef ENABLE_DEBUG
    printf("Data: %d\n", node->data);
#endif
    return BT_IR__CONTINUE;
}

void deleter_ut(bt_node_t *node, void *user_data) {
#ifdef ENABLE_DEBUG
    printf("Deleting node holding data(%d)\n", node->data);
#endif
    free(node);
}

int8_t comp( void *d1, void *d2 ) {
    if( (int)d1 > (int)d2 ) {
        return 1;
    } else if( d1 == d2 ) {
        return 0;
    }
    return -1;
}

void create_node_and_add_to_list( bt_list_t *list, int data, bool isExpected ) {
    bt_node_t *node = malloc(sizeof(bt_node_t));
    bool rv;
    CU_ASSERT_FATAL(NULL != node);
    bt_init_node(node, (void*)data);

    rv = bt_add(list, node);
    CU_ASSERT( isExpected == rv );
    if( !rv ) {
        free(node);
    }
}

void test_bt_add( void )
{
    int ii;
    bt_list_t list;
    bt_node_t *node;

    bt_init_list(&list, comp);

    /* Test bad inputs */
    CU_ASSERT( false == bt_add(&list, NULL) );
    create_node_and_add_to_list(NULL, 0, false);

    for( ii = 1; ii <= 9; ii++ ) {
        create_node_and_add_to_list(&list, ii, true);
    }

    for( ii = -1; ii >= -9; ii-- ) {
        create_node_and_add_to_list(&list, ii, true);
    }

    /* Now for the stuff which causes a double rotate to occur */
    create_node_and_add_to_list(&list, 15, true);
    create_node_and_add_to_list(&list, 16, true);
    create_node_and_add_to_list(&list, 12, true);
    create_node_and_add_to_list(&list, 13, true);
    create_node_and_add_to_list(&list, -19, true);
    create_node_and_add_to_list(&list, -13, true);
    create_node_and_add_to_list(&list, -18, true);
    create_node_and_add_to_list(&list, -14, true);
    create_node_and_add_to_list(&list, -17, true);
    create_node_and_add_to_list(&list, -16, true);
    create_node_and_add_to_list(&list, -15, true);

    /* Make sure that we get a false if the node already
     * exists
     */
    create_node_and_add_to_list(&list, 8, false);
    create_node_and_add_to_list(&list, -15, false);

    bt_iterate(&list, iterate_print, NULL, NULL);

    node = bt_find(&list, (void*)12);
    CU_ASSERT( (void*)12 == node->data );
    node = bt_find(NULL, (void*)15);
    CU_ASSERT( NULL == node );
    node = bt_find(&list, NULL);
    CU_ASSERT( NULL == node );
    node = bt_find(&list, (void*)100);
    CU_ASSERT( NULL == node );

    bt_delete_list(&list, deleter_ut, NULL);
    bt_iterate(&list, iterate_print, NULL, NULL);
}

void test_bt_head_tail() {
    bt_list_t list;
    bt_node_t *node;
    bt_init_list(&list, comp);

    node = bt_get_head(NULL);
    CU_ASSERT(NULL == node );

    node = bt_get_tail(NULL);
    CU_ASSERT(NULL == node );

    node = bt_get_head(&list);
    CU_ASSERT(NULL == node );

    node = bt_get_tail(&list);
    CU_ASSERT(NULL == node );

    create_node_and_add_to_list(&list, 1, true);

    node = bt_get_head(&list);
    CU_ASSERT( (void*)1 == node->data );

    node = bt_get_tail(&list);
    CU_ASSERT( (void*)1 == node->data );

    create_node_and_add_to_list(&list, -1, true);

    node = bt_get_head(&list);
    CU_ASSERT( (void*)-1 == node->data );

    node = bt_get_tail(&list);
    CU_ASSERT( (void*)1 == node->data );

    create_node_and_add_to_list(&list, 2, true);
    node = bt_get_head(&list);
    CU_ASSERT( (void*)-1 == node->data );

    node = bt_get_tail(&list);
    CU_ASSERT( (void*)2 == node->data );

    bt_delete_list(&list, deleter_ut, NULL);
}

void test_bt_get( void )
{
    bt_list_t list;
    bt_node_t *node = NULL;
    bt_init_list(&list, comp);

    node = bt_get(NULL, node, BT_GET__NEXT);
    CU_ASSERT( NULL == node );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT( NULL == node );

    create_node_and_add_to_list(&list, 1, true);
    node = bt_get(&list, NULL, BT_GET__NEXT);
    CU_ASSERT( NULL == node );

    node = bt_get(&list, bt_get_head(&list), BT_GET__NEXT);
    CU_ASSERT( NULL == node );

    node = bt_get(&list, bt_get_head(&list), BT_GET__PREVIOUS);
    CU_ASSERT( NULL == node );

    create_node_and_add_to_list(&list, 3, true);
    create_node_and_add_to_list(&list, -2, true);

    node = bt_get(&list, bt_get_tail(&list), BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)1 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)-2 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)1 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)3 == node->data );

    create_node_and_add_to_list(&list, 2, true);
    create_node_and_add_to_list(&list, -1, true);

    node = bt_get(&list, bt_get_tail(&list), BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)2 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)1 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)-1 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)-2 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)-1 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)1 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)2 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( (void*)3 == node->data );

    {
        bt_node_t node_not_in_list;
        bt_init_node(&node_not_in_list, (void*)-3);
        node = bt_get(&list, &node_not_in_list, BT_GET__NEXT);
        CU_ASSERT( NULL == node );
    }

    {
        bt_node_t node_not_in_list;
        bt_init_node(&node_not_in_list, (void*)4);
        node = bt_get(&list, &node_not_in_list, BT_GET__PREVIOUS);
        CU_ASSERT( NULL == node );
    }

    bt_delete_list(&list, deleter_ut, NULL);
}


typedef struct {
    size_t counter;
    size_t array_size;
    int *array;
    bool didFailureOccur;
} __check_list_t;

static bt_ir_t __check_list_iterator(bt_node_t *node, void *user_data)
{
    __check_list_t *data = (__check_list_t *)user_data;

    CU_ASSERT_FATAL( data->counter < data->array_size );
    CU_ASSERT( data->array[data->counter] == (int)node->data );
    if( data->array[data->counter] != (int)node->data ) {
        data->didFailureOccur = true;
    }
    data->counter++;
    return BT_IR__CONTINUE;
}

static bool __check_list( bt_list_t *list, int *array, size_t array_size )
{
    __check_list_t data;
    data.counter = 0;
    data.array_size = array_size;
    data.array = array;
    data.didFailureOccur = false;

    bt_iterate(list, __check_list_iterator, NULL, &data);
    return !data.didFailureOccur;
}

void test_bt_remove( void )
{
    bt_list_t list;

    bt_init_list(&list, comp);

    bt_remove(NULL, (void*)1, deleter_ut, NULL );
    bt_remove(&list, NULL, deleter_ut, NULL);
    bt_remove(&list, (void*)1, NULL, NULL);
    bt_remove(&list, (void*)1, deleter_ut, NULL);

    create_node_and_add_to_list(&list, 5, true);
    bt_remove(&list, (void*)1, deleter_ut, NULL);

    bt_remove(&list, (void*)5, deleter_ut, NULL);
    CU_ASSERT( NULL == bt_get_head(&list) );

    create_node_and_add_to_list(&list, 5, true);
    create_node_and_add_to_list(&list, 4, true);
    create_node_and_add_to_list(&list, 6, true);
    create_node_and_add_to_list(&list, 7, true);

    /* remove: left NULL, right DATA */
    bt_remove(&list, (void*)6, deleter_ut, NULL);
    {
        int local_array[] = {4,5,7};
        CU_ASSERT(__check_list( &list, local_array, sizeof(local_array) ));
    }

    create_node_and_add_to_list(&list, 6, true);
    /* remove: left DATA, right NULL */
    bt_remove(&list, (void*)7, deleter_ut, NULL);
    {
        int local_array[] = {4,5,6};
        CU_ASSERT(__check_list( &list, local_array, sizeof(local_array) ));
    }

    bt_delete_list(&list, deleter_ut, NULL);
    create_node_and_add_to_list(&list, 5, true);
    create_node_and_add_to_list(&list, 4, true);
    create_node_and_add_to_list(&list, 7, true);
    create_node_and_add_to_list(&list, 6, true);
    create_node_and_add_to_list(&list, 8, true);
    bt_remove(&list, (void*)7, deleter_ut, NULL);
    /* remove: left DATA, right DATA...no balance required */
    {
        int local_array[] = {4,5,6,8};
        CU_ASSERT(__check_list( &list, local_array, sizeof(local_array) ));
    }

    bt_delete_list(&list, deleter_ut, NULL);
    create_node_and_add_to_list(&list, 5, true);
    create_node_and_add_to_list(&list, 4, true);
    create_node_and_add_to_list(&list, 7, true);
    create_node_and_add_to_list(&list, 6, true);
    create_node_and_add_to_list(&list, 8, true);
    bt_remove(&list, (void*)5, deleter_ut, NULL);
    /* remove: left DATA, right DATA...no balance required */
    {
        int local_array[] = {4,6,7,8};
        CU_ASSERT(__check_list( &list, local_array, sizeof(local_array) ));
    }

    bt_delete_list(&list, deleter_ut, NULL);
    create_node_and_add_to_list(&list, 5, true);
    create_node_and_add_to_list(&list, 4, true);
    create_node_and_add_to_list(&list, 7, true);
    create_node_and_add_to_list(&list, 6, true);
    create_node_and_add_to_list(&list, 8, true);
    bt_remove(&list, (void*)4, deleter_ut, NULL);
    /* remove: left NULL, right NULL...balance required */
    {
        int local_array[] = {5,6,7,8};
        CU_ASSERT(__check_list( &list, local_array, sizeof(local_array) ));
    }


}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Singly Binary Tree (AVL) Test", NULL, NULL );
    CU_add_test( *suite, "Test bt_add()", test_bt_add );
    CU_add_test( *suite, "Test bt_head/tail()", test_bt_head_tail );
    CU_add_test( *suite, "Test bt_get()", test_bt_get );
    CU_add_test( *suite, "Test bt_remove()", test_bt_remove );
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
