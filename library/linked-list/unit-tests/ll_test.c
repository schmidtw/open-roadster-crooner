#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "../src/linked-list.h"

void test_list_append( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3;

    memset( &node1, 55, sizeof(ll_node_t) );
    memset( &node2, 55, sizeof(ll_node_t) );
    memset( &node3, 55, sizeof(ll_node_t) );

    CU_ASSERT_PTR_NOT_NULL( node1.next );
    CU_ASSERT_PTR_NOT_NULL( node2.next );
    CU_ASSERT_PTR_NOT_NULL( node3.next );

    ll_append( NULL, NULL );

    ll_init_list( &list );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_append( &list, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_append( &list, &node1 );

    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );
    CU_ASSERT_PTR_NULL( node1.next );

    ll_append( &list, &node2 );

    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node2 );
    CU_ASSERT_PTR_EQUAL( node1.next, &node2 );
    CU_ASSERT_PTR_NULL( node2.next );

    ll_append( &list, &node3 );

    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node3 );
    CU_ASSERT_PTR_EQUAL( node1.next, &node2 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node3 );
    CU_ASSERT_PTR_NULL( node3.next );
}

void test_list_prepend( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3;

    memset( &node1, 55, sizeof(ll_node_t) );
    memset( &node2, 55, sizeof(ll_node_t) );
    memset( &node3, 55, sizeof(ll_node_t) );

    CU_ASSERT_PTR_NOT_NULL( node1.next );
    CU_ASSERT_PTR_NOT_NULL( node2.next );
    CU_ASSERT_PTR_NOT_NULL( node3.next );

    ll_prepend( NULL, NULL );

    ll_init_list( &list );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_prepend( &list, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_prepend( &list, &node1 );

    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );
    CU_ASSERT_PTR_NULL( node1.next );

    ll_prepend( &list, &node2 );

    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node1 );
    CU_ASSERT_PTR_NULL( node1.next );

    ll_prepend( &list, &node3 );

    CU_ASSERT_PTR_EQUAL( list.head, &node3 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );
    CU_ASSERT_PTR_EQUAL( node3.next, &node2 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node1 );
    CU_ASSERT_PTR_NULL( node1.next );
}

void test_list_insert( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3, node4, node5, node6;

    ll_init_node( &node1, NULL );
    ll_init_node( &node2, NULL );
    ll_init_node( &node3, NULL );
    ll_init_node( &node4, NULL );
    ll_init_node( &node5, NULL );
    ll_init_node( &node6, NULL );

    ll_insert_after( NULL, NULL, NULL );

    ll_init_list( &list );
    ll_insert_after( &list, NULL, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_insert_after( &list, NULL, &node1 );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    /* No nodes in the list, but choose a random one as the insertion point. */
    ll_insert_after( &list, &node1, &node2 );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    /* List: <empty>
     */
    ll_prepend( &list, &node1 );
    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );
    CU_ASSERT_PTR_NULL( node1.next );

    /* List: node1 -> node2, node1
     */
    ll_insert_after( &list, &node2, NULL );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node1 );
    CU_ASSERT_PTR_NULL( node1.next );

    /* List: node2, node1 -> node2, node1, node3
     */
    ll_insert_after( &list, &node3, &node1 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node3 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node1 );
    CU_ASSERT_PTR_EQUAL( node1.next, &node3 );
    CU_ASSERT_PTR_NULL( node3.next );

    /* List: node2, node1, node3 -> node2, node4, node1, node3
     */
    ll_insert_after( &list, &node4, &node2 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node3 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node4 );
    CU_ASSERT_PTR_EQUAL( node4.next, &node1 );
    CU_ASSERT_PTR_EQUAL( node1.next, &node3 );
    CU_ASSERT_PTR_NULL( node3.next );

    /* List: node2, node4, node1, node3 -> node2, node4, node1, node5, node3
     */
    ll_insert_after( &list, &node5, &node1 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node3 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node4 );
    CU_ASSERT_PTR_EQUAL( node4.next, &node1 );
    CU_ASSERT_PTR_EQUAL( node1.next, &node5 );
    CU_ASSERT_PTR_EQUAL( node5.next, &node3 );
    CU_ASSERT_PTR_NULL( node3.next );

    /* List: node2, node4, node1, node5, node3 -> node2, node4, node1, node5, node3
     * Invalid node as the insertion point.
     */
    ll_insert_after( &list, &node6, &node6 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node3 );
    CU_ASSERT_PTR_EQUAL( node2.next, &node4 );
    CU_ASSERT_PTR_EQUAL( node4.next, &node1 );
    CU_ASSERT_PTR_EQUAL( node1.next, &node5 );
    CU_ASSERT_PTR_EQUAL( node5.next, &node3 );
    CU_ASSERT_PTR_NULL( node3.next );
}

ll_ir_t iterate1( ll_node_t *node, volatile void *user_data )
{
    CU_FAIL( "iterate1 should not be called\n" );
    return LL_IR__CONTINUE;
}

ll_ir_t iterate2( ll_node_t *node, volatile void *user_data )
{
    CU_ASSERT_PTR_NULL( user_data );
    CU_ASSERT_PTR_NULL( node->next );
    CU_PASS( "iterate2 called.\n" );
    return LL_IR__CONTINUE;
}

static int iterate3_call_count = 0;
ll_ir_t iterate3( ll_node_t *node, volatile void *user_data )
{
    switch( iterate3_call_count++ ) {
        case 0:
            return LL_IR__CONTINUE;
        case 1:
            return LL_IR__DELETE_AND_CONTINUE;
        case 2:
            return LL_IR__DELETE_AND_CONTINUE;
        case 3:
            return LL_IR__CONTINUE;
        default:
            break;
    }

    return LL_IR__CONTINUE;
}

static int iterate4_call_count = 0;
ll_ir_t iterate4( ll_node_t *node, volatile void *user_data )
{
    switch( iterate4_call_count++ ) {
        case 0:
            return LL_IR__DELETE_AND_CONTINUE;
        case 1:
            return LL_IR__CONTINUE;
        case 2:
            return LL_IR__CONTINUE;
        case 3:
            return LL_IR__DELETE_AND_CONTINUE;
        default:
            break;
    }

    return LL_IR__CONTINUE;
}

static int iterate5_call_count = 0;
ll_ir_t iterate5( ll_node_t *node, volatile void *user_data )
{
    switch( iterate5_call_count++ ) {
        case 0:
            return LL_IR__STOP;
        default:
            break;
    }

    return LL_IR__CONTINUE;
}

static int iterate6_call_count = 0;
ll_ir_t iterate6( ll_node_t *node, volatile void *user_data )
{
    switch( iterate6_call_count++ ) {
        case 0:
            return LL_IR__DELETE_AND_CONTINUE;
        case 1:
            return LL_IR__DELETE_AND_STOP;
        default:
            break;
    }

    return LL_IR__DELETE_AND_CONTINUE;
}

void destroy1( ll_node_t *node, volatile void *user_data )
{
    CU_FAIL( "destroy1 should not be called\n" );
}

static int destroy2_call_count = 0;
void destroy2( ll_node_t *node, volatile void *user_data )
{
    destroy2_call_count++;
}

void test_list_iterate( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3, node4;

    ll_init_node( &node1, NULL );
    ll_init_node( &node2, NULL );
    ll_init_node( &node3, NULL );
    ll_init_node( &node4, NULL );

    ll_iterate( NULL, NULL, NULL, NULL );
    ll_iterate( NULL, iterate1, NULL, NULL );
    ll_iterate( NULL, iterate1, destroy1, NULL );

    ll_init_list( &list );
    ll_iterate( &list, NULL, NULL, NULL );
    ll_iterate( &list, iterate1, NULL, NULL );
    ll_iterate( &list, iterate1, destroy1, NULL );

    ll_append( &list, &node1 );
    ll_iterate( &list, NULL, NULL, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_append( &list, &node1 );
    ll_iterate( &list, iterate2, NULL, NULL );
    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node1 );

    ll_append( &list, &node2 );
    ll_append( &list, &node3 );
    ll_append( &list, &node4 );

    /* List: node1, node2, node3, node4 -> node1, node4 */
    ll_iterate( &list, iterate3, destroy2, NULL );
    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node4 );
    CU_ASSERT( 2 == destroy2_call_count );
    CU_ASSERT( 4 == iterate3_call_count );

    /* Clear the list */
    ll_iterate( &list, NULL, NULL, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_append( &list, &node1 );
    ll_append( &list, &node2 );
    ll_append( &list, &node3 );
    ll_append( &list, &node4 );

    /* List: node1, node2, node3, node4 -> node2, node3 */
    destroy2_call_count = 0;
    ll_iterate( &list, iterate4, destroy2, NULL );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node3 );
    CU_ASSERT( 2 == destroy2_call_count );
    CU_ASSERT( 4 == iterate3_call_count );

    /* Clear the list */
    ll_iterate( &list, NULL, NULL, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );

    ll_append( &list, &node1 );
    ll_append( &list, &node2 );
    ll_append( &list, &node3 );
    ll_append( &list, &node4 );

    /* List: node1, node2, node3, node4 -> (unchanged) */
    ll_iterate( &list, iterate5, NULL, NULL );
    CU_ASSERT( 1 == iterate5_call_count );

    /* List: node1, node2, node3, node4 -> node3, node4 */
    ll_iterate( &list, iterate6, NULL, NULL );
    CU_ASSERT( 2 == iterate6_call_count );
    CU_ASSERT_PTR_EQUAL( list.head, &node3 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node4 );
}

void test_list_delete_list( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3, node4;

    ll_delete_list( NULL, NULL, NULL );

    ll_init_list( &list );
    ll_append( &list, &node1 );
    ll_append( &list, &node2 );
    ll_append( &list, &node3 );
    ll_append( &list, &node4 );

    ll_delete_list( NULL, NULL, NULL );

    ll_delete_list( &list, NULL, NULL );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );
}

void test_list_remove( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3, node4;

    ll_remove( NULL, NULL );

    ll_init_list( &list );
    ll_append( &list, &node1 );
    ll_append( &list, &node2 );
    ll_append( &list, &node3 );
    ll_append( &list, &node4 );

    /* List: node1, node2, node3, node4 -> (unchanged) */
    ll_remove( &list, NULL );
    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node2 );
    CU_ASSERT_PTR_EQUAL( list.head->next->next, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head->next->next->next, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next->next->next->next );

    /* List: node1, node2, node3, node4 -> node2, node3, node4 */
    ll_remove( &list, &node1 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head->next->next, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next->next->next );

    /* List: node2, node3, node4 -> node2, node4 */
    ll_remove( &list, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next->next );

    /* List: node2, node4 -> node2 */
    ll_remove( &list, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node2 );
    CU_ASSERT_PTR_NULL( list.head->next );

    /* List: node2, -> (empty) */
    ll_remove( &list, &node2 );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );
}

void test_list_remove_head( void )
{
    ll_list_t list;
    ll_node_t node1, node2, node3, node4;
    ll_node_t *n;

    ll_remove( NULL, NULL );

    ll_init_list( &list );
    ll_append( &list, &node1 );
    ll_append( &list, &node2 );
    ll_append( &list, &node3 );
    ll_append( &list, &node4 );

    /* List: node1, node2, node3, node4 -> (unchanged) */
    n = ll_remove_head( NULL );
    CU_ASSERT_PTR_NULL( n );
    CU_ASSERT_PTR_EQUAL( list.head, &node1 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node2 );
    CU_ASSERT_PTR_EQUAL( list.head->next->next, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head->next->next->next, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next->next->next->next );

    /* List: node1, node2, node3, node4 -> node2, node3, node4 */
    n = ll_remove_head( &list );
    CU_ASSERT_PTR_EQUAL( n, &node1 );
    CU_ASSERT_PTR_EQUAL( list.head, &node2 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head->next->next, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next->next->next );

    /* List: node2, node3, node4 -> node3, node4 */
    n = ll_remove_head( &list );
    CU_ASSERT_PTR_EQUAL( n, &node2 );
    CU_ASSERT_PTR_EQUAL( list.head, &node3 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_EQUAL( list.head->next, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next->next );

    /* List: node3, node4 -> node4 */
    n = ll_remove_head( &list );
    CU_ASSERT_PTR_EQUAL( n, &node3 );
    CU_ASSERT_PTR_EQUAL( list.head, &node4 );
    CU_ASSERT_PTR_EQUAL( list.tail, &node4 );
    CU_ASSERT_PTR_NULL( list.head->next );

    /* List: node2, -> (empty) */
    n = ll_remove_head( &list );
    CU_ASSERT_PTR_EQUAL( n, &node4 );
    CU_ASSERT_PTR_NULL( list.head );
    CU_ASSERT_PTR_NULL( list.tail );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Singly Linked List Test", NULL, NULL );
    CU_add_test( *suite, "Test ll_append()     ", test_list_append );
    CU_add_test( *suite, "Test ll_prepend()    ", test_list_prepend );
    CU_add_test( *suite, "Test ll_insert()     ", test_list_insert );
    CU_add_test( *suite, "Test ll_iterate()    ", test_list_iterate );
    CU_add_test( *suite, "Test ll_delete_list()", test_list_delete_list );
    CU_add_test( *suite, "Test ll_remove()     ", test_list_remove );
    CU_add_test( *suite, "Test ll_remove_head()", test_list_remove_head );
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
