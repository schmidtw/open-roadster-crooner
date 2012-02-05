#include <CUnit/Basic.h>
#include <stdio.h>
#include <stdlib.h>
#include "database.h"
#include "internal_database.h"
#include "database_print.h"
#include "add_song.h"
#include "generic.h"
#include "indexer.h"

root_database_node_t rdn;

media_status_t fake_play( generic_node_t * root,
                          media_metadata_t * metadata,
                          media_play_fn_t play_fn,
                          char * file_location )
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
    
    node = get_new_generic_node(GNT_ROOT, "root");
    CU_ASSERT( NULL != node );
    rdn.initialized = true;
    rdn.root = (generic_node_t*)node->data;
    group = rdn.root;
    
    setup_metadata(&metadata, "A Me", "Brushfire Fairytales", "Inaudible Melodies", 1);

    so_n = add_song_to_root(group, &metadata, fake_play, "Here" );
    CU_ASSERT( NULL != so_n );
    
    setup_metadata(&metadata, "Jack Johnson", "Brushfire Fairytales", "Inaudible Melodies", 2);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Jack Johnson", "Brushfire Fairytales", "Middle Man", 1);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );
    
    setup_metadata(&metadata, "Jack Johnson", "In between dreams", "Better Together", 1);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );
    setup_metadata(&metadata, "Jack Johnson", "In between dreams", "Never Know", 2);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Frank Sinatra", 1);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "The Distance", 2);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Friend is a Four Letter Word", 3);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Open Book", 4);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Daria", 5);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Race Car Ya-Yas", 6);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "I Will Survive", 7);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Stickshifts and Safetybelts", 8);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "Perhaps, Perhaps, Perhaps", 9);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    setup_metadata(&metadata, "Cake", "Fashion Nugget", "It's Coming Down", 10);
    so_n = add_song_to_root(group, &metadata, fake_play, "Here");
    CU_ASSERT( NULL != so_n );

    index_root( &(rdn.root->node) );
}

void print_song_info( song_node_t * node )
{
    if( NULL == node ) {
        printf("Oops, the song is NULL\n");
    } else {
        printf( "%s -> %s -> %s -> %s\n",
                node->d.parent->parent->parent->name.root,
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

void test_next_song( void )
{
    song_node_t * so_n = NULL;
    create_simple_database();
    printf("\n");
    CU_ASSERT( DS_FAILURE == next_song(NULL, DT_NEXT, DL_SONG) );

    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_SONG) );
    printf("First Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_NEXT, DL_SONG) );
    printf("Next Song: ");
    print_song_info( so_n );

    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_ARTIST) );
    printf("Next Artist: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_NEXT, DL_SONG) );
    printf("Next Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_NEXT, DL_ALBUM) );
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
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    printf("Previous Artist: ");
    print_song_info( so_n );
    CU_ASSERT( DS_END_OF_LIST == next_song(&so_n, DT_PREVIOUS, DL_ALBUM) );
    printf("Previous Album: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_PREVIOUS, DL_ALBUM) );
    printf("Previous Artist: ");
    print_song_info( so_n );
    database_purge();
}

void test_random_song( void )
{
    int ii;
    song_node_t * so_n = NULL;
    create_simple_database();
    printf("\n");
    CU_ASSERT( DS_FAILURE == next_song(NULL, DT_RANDOM, DL_SONG) );
    srand(11);
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_SONG) );
    printf("Random Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_ALBUM) );
    printf("Random Album: ");
    print_song_info( so_n );
    for( ii = 0; ii < 10; ii++ ) {
        CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_RANDOM, DL_ARTIST) );
        printf("Random Artist: ");
        print_song_info( so_n );
    }

    database_purge();
}

void test_queued_song( void )
{
    int ii;
    song_node_t * so_n = NULL;
    song_node_t * backup1_n, *backup2_n, *backup3_n, *backup4_n, *backup5_n;
    queued_song_init();
    create_simple_database();
    printf("\n");
    srand(11);

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    backup1_n = so_n;
    print_song_info( so_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_NEXT, DL_ARTIST) );
    backup2_n = so_n;
    print_song_info( so_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_NEXT, DL_SONG) );
    backup3_n = so_n;
    print_song_info( so_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_NEXT, DL_SONG) );
    backup4_n = so_n;
    print_song_info( so_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_NEXT, DL_SONG) );
    backup5_n = so_n;
    print_song_info( so_n );


    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_NEXT, DL_SONG) );
    print_song_info( so_n );

    CU_ASSERT( backup2_n != backup1_n );
    CU_ASSERT( backup3_n != backup2_n );
    CU_ASSERT( backup4_n != backup3_n );
    CU_ASSERT( backup5_n != backup4_n );
    CU_ASSERT( so_n != backup5_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    print_song_info( so_n );
    CU_ASSERT( so_n == backup5_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    print_song_info( so_n );
    CU_ASSERT( so_n == backup4_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    print_song_info( so_n );
    CU_ASSERT( so_n == backup3_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    CU_ASSERT( so_n == backup2_n );
    print_song_info( so_n );

    CU_ASSERT( DS_FAILURE != queued_next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
    CU_ASSERT( so_n == backup1_n );
    print_song_info( so_n );

    database_purge();
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Print Test", NULL, NULL );
    CU_add_test( *suite, "Test Simple Print   ", test_simple_test );
    CU_add_test( *suite, "Test Next Song Print", test_next_song );
    CU_add_test( *suite, "Test Prev Song Print", test_previous_song );
    CU_add_test( *suite, "Test Rand Song Print", test_random_song );
    CU_add_test( *suite, "Test Queued Song", test_queued_song );

    database_purge();
}

int main( int argc, char *argv[] )
{
    int rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();
    }

    if( 0 != rv ) {
        return 1;
    }
    return CU_get_error();
}
