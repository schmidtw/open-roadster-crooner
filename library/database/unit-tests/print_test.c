#include <CUnit/Basic.h>
#include <stdio.h>
#include "database.h"
#include "internal_database.h"
#include "database_print.h"
#include "add_song.h"
#include "generic.h"

root_database_node_t rdn;

media_status_t fake_play( const char *filename,
                          const double gain,
                          const double peak,
                          xQueueHandle idle,
                          const size_t queue_size,
                          media_malloc_fn_t malloc_fn,
                          media_free_fn_t free_fn,
                          media_command_fn_t command_fn )
{
    return MI_RETURN_OK;
}

void setup_metadata( media_metadata_t * metadata, char * artist, char * album, char * song, uint32_t track_num )
{
    if( NULL == metadata ) {
        printf("The Metadata pointer must not be NULL\n");
    }
    CU_ASSERT(NULL != metadata);
    bzero(metadata, sizeof(media_metadata_t));

    if( NULL != artist ) {
        sprintf(metadata->artist, "%s", artist);
    }
    if( NULL != album ) {
        sprintf(metadata->album, "%s", album);
    }
    if( NULL != song ) {
        sprintf(metadata->title, "%s", song);
    }
    metadata->track_number = track_num;
}

void create_simple_database( void )
{
    generic_node_t * group;
    song_node_t * so_n;
    ll_node_t * node;
    media_metadata_t metadata;
    
    node = get_new_generic_node(GNT_GROUP, "1");
    CU_ASSERT( NULL != node );
    ll_append( &rdn.groups, node );
    rdn.size_list++;
    rdn.initialized = true;
    
    group = (generic_node_t *)rdn.groups.head->data;
    
    setup_metadata(&metadata, "A Me", "Brushfire Fairytales", "Inaudible Melodies", 1);

    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    
    setup_metadata(&metadata, "Jack Johnson", "Brushfire Fairytales", "Inaudible Melodies", 2);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Jack Johnson", "Brushfire Fairytales", "Middle Man", 1);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );
    
    setup_metadata(&metadata, "Jack Johnson", "In between dreams", "Better Together", 1);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );
    setup_metadata(&metadata, "Jack Johnson", "In between dreams", "Never Know", 2);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );
}

void print_song_info( song_node_t * node )
{
    if( NULL == node ) {
        printf("Oops, the song is NULL\n");
    } else {
        printf( "%s -> %s -> %s -> %s\n",
                node->d.parent->parent->parent->name.group,
                node->d.parent->parent->name.artist,
                node->d.parent->name.album,
                node->d.name.song );
    }
}

void test_simple_test( void )
{
    database_purge();
    create_simple_database();
    database_print();
    database_purge();
    database_print();
}

void test_group_test( void )
{
    generic_node_t * group;
    song_node_t * so_n;
    ll_node_t * node;
    media_metadata_t metadata;
    
    database_purge();

    node = get_new_generic_node(GNT_GROUP, "1");
    CU_ASSERT( NULL != node );
    ll_append( &rdn.groups, node );
    rdn.size_list++;
    node = get_new_generic_node(GNT_GROUP, "2");
    CU_ASSERT( NULL != node );
    ll_append( &rdn.groups, node );
    rdn.size_list++;
    rdn.initialized = true;
    
    group = (generic_node_t *)rdn.groups.head->data;
    
    setup_metadata(&metadata, "A Me", "Brushfire Fairytales", "Inaudible Melodies", 1);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    
    setup_metadata(&metadata, "Jack Johnson", "Brushfire Fairytales", "Inaudible Melodies", 2);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    setup_metadata(&metadata, "Jack Johnson", "Brushfire Fairytales", "Middle Man", 1);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    /* Add the exact same song again */
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    
    /* Add the same song with different gain */
    metadata.track_gain = 1;
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );

    group = (generic_node_t *)rdn.groups.head->next->data;
    setup_metadata(&metadata, "Jack Johnson", "In between dreams", "Better Together", 1);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    setup_metadata(&metadata, "Jack Johnson", "In between dreams", "Never Know", 2);
    so_n = add_song_to_group(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );

    database_print();
    database_purge();
    database_print();
}

void test_next_song( void )
{
    song_node_t * so_n = NULL;
    create_simple_database();
    printf("\n");
    CU_ASSERT( DS_FAILURE == next_song(NULL, DT_NEXT, DL_SONG) );

    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_SONG) );
    printf("First Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_ARTIST) );
    printf("Next Artist: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_SONG) );
    printf("Next Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_ALBUM) );
    printf("Next Album: ");
    print_song_info( so_n );
    database_purge();
}

void test_previous_song( void )
{
    song_node_t * so_n = NULL;
    create_simple_database();
    printf("\n");
    CU_ASSERT( DS_FAILURE == next_song(NULL, DT_PREVIOUS, DL_SONG) );
    
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_PREVIOUS, DL_SONG) );
    printf("Previous Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_PREVIOUS, DL_SONG) );
    printf("Previous Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_PREVIOUS, DL_ALBUM) );
    printf("Previous Album: ");
    print_song_info( so_n );
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    printf("Previous Artist: ");
    print_song_info( so_n );
    database_purge();
}

void test_random_song( void )
{
    song_node_t * so_n = NULL;
    create_simple_database();
    printf("\n");
    CU_ASSERT( DS_FAILURE == next_song(NULL, DT_RANDOM, DL_SONG) );
    srand(11);
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_SONG) );
    printf("Random Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_GROUP) );
    printf("Random Group: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_GROUP) );
    printf("Random Group: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_GROUP) );
    printf("Random Group: ");
    print_song_info( so_n );
    database_purge();
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Print Test", NULL, NULL );
    CU_add_test( *suite, "Test Simple Print   ", test_simple_test );
    CU_add_test( *suite, "Test Group Print    ", test_group_test );
    CU_add_test( *suite, "Test Next Song Print", test_next_song );
    CU_add_test( *suite, "Test Prev Song Print", test_previous_song );
    CU_add_test( *suite, "Test Rand Song Print", test_random_song );
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
