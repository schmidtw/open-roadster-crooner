#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "../src/binary-tree-avl.h"

//#define ENABLE_DEBUG 1

bt_ir_t iterate_print( bt_node_t *node, volatile void *user_data )
{
#ifdef ENABLE_DEBUG
    printf("Data: %d\n", node->data);
#endif
    return BT_IR__CONTINUE;
}

void deleter_ut(bt_node_t *node, volatile void *user_data) {
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
    bt_init_node(node, data);

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

    node = bt_find(&list, 12);
    CU_ASSERT( 12 == node->data );
    node = bt_find(NULL, 15);
    CU_ASSERT( NULL == node );
    node = bt_find(&list, NULL);
    CU_ASSERT( NULL == node );
    node = bt_find(&list, 100);
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
    CU_ASSERT( 1 == node->data );

    node = bt_get_tail(&list);
    CU_ASSERT( 1 == node->data );

    create_node_and_add_to_list(&list, -1, true);

    node = bt_get_head(&list);
    CU_ASSERT( -1 == node->data );

    node = bt_get_tail(&list);
    CU_ASSERT( 1 == node->data );

    create_node_and_add_to_list(&list, 2, true);
    node = bt_get_head(&list);
    CU_ASSERT( -1 == node->data );

    node = bt_get_tail(&list);
    CU_ASSERT( 2 == node->data );

    bt_delete_list(&list, deleter_ut, NULL);
}

void test_bt_get( void )
{
    bt_list_t list;
    bt_node_t *node;
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
    CU_ASSERT( 1 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( -2 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 1 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 3 == node->data );

    create_node_and_add_to_list(&list, 2, true);
    create_node_and_add_to_list(&list, -1, true);

    node = bt_get(&list, bt_get_tail(&list), BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 2 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 1 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( -1 == node->data );

    node = bt_get(&list, node, BT_GET__PREVIOUS);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( -2 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( -1 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 1 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 2 == node->data );

    node = bt_get(&list, node, BT_GET__NEXT);
    CU_ASSERT_FATAL( NULL != node );
    CU_ASSERT( 3 == node->data );

    {
        bt_node_t node_not_in_list;
        bt_init_node(&node_not_in_list, -3);
        node = bt_get(&list, &node_not_in_list, BT_GET__NEXT);
        CU_ASSERT( NULL == node );
    }

    {
        bt_node_t node_not_in_list;
        bt_init_node(&node_not_in_list, 4);
        node = bt_get(&list, &node_not_in_list, BT_GET__PREVIOUS);
        CU_ASSERT( NULL == node );
    }

    bt_delete_list(&list, deleter_ut, NULL);
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Singly Binary Tree (AVL) Test", NULL, NULL );
    CU_add_test( *suite, "Test bt_add()", test_bt_add );
    CU_add_test( *suite, "Test bt_head/tail()", test_bt_head_tail );
    CU_add_test( *suite, "Test bt_get()", test_bt_get );
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