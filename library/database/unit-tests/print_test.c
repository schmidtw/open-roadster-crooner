#include <CUnit/Basic.h>
#include <stdio.h>
#include "database.h"
#include "internal_database.h"
#include "database_print.h"
#include "add_song.h"
#include "group.h"

root_database_node_t rdn;

char artist[MAX_ARTIST_NAME_W_NULL];
char album[MAX_ALBUM_TITLE_W_NULL];
char song[MAX_SONG_TITLE_W_NULL];

void fake_audio_control( void )
{
    printf("fake_audio_control called\n");
}

void create_simple_database( void )
{
    group_node_t * group;
    song_node_t * so_n;
    ll_node_t * node;
    
    node = get_new_group_and_node("1");
    CU_ASSERT( NULL != node );
    ll_append( &rdn.groups, node );
    rdn.size_list++;
    rdn.initialized = true;
    
    group = (group_node_t *)rdn.groups.head->data;
    
    sprintf(artist, "A Me");
    sprintf(album, "Brushfire Fairytales");
    sprintf(song, "Inaudible Melodies");
    so_n = add_song_to_group(group, artist, album, song, 1, 0, 0, 0, 0, fake_audio_control, "Here" );
    CU_ASSERT( NULL != so_n );
    
    sprintf(artist, "Jack Johnson");
    sprintf(album, "Brushfire Fairytales");
    sprintf(song, "Inaudible Melodies");
    so_n = add_song_to_group(group, artist, album, song, 2, 0, 0, 0, 0, fake_audio_control, "Here");
    CU_ASSERT( NULL != so_n );
    sprintf(song, "Middle Man");
    so_n = add_song_to_group(group, artist, album, song, 1, 0, 0, 0, 0, fake_audio_control, "Here");
    CU_ASSERT( NULL != so_n );
    
    sprintf(album, "In between dreams");
    sprintf(song, "Better Together");
    so_n = add_song_to_group(group, artist, album, song, 1, 0, 0, 0, 0, fake_audio_control, "Here");
    CU_ASSERT( NULL != so_n );
    sprintf(song, "Never Know");
    so_n = add_song_to_group(group, artist, album, song, 2, 0, 0, 0, 0, fake_audio_control, "Here");
    CU_ASSERT( NULL != so_n );
}

void print_song_info( song_node_t * node )
{
    if( NULL == node ) {
        printf("Oops, the song is NULL\n");
    } else {
        printf( "%s -> %s -> %s -> %s\n",
                node->album->artist->group->name,
                node->album->artist->name,
                node->album->name,
                node->title );
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
    
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_PREVIOUS, DL_SONG) );
    printf("Previous Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_PREVIOUS, DL_SONG) );
    printf("Previous Song: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_PREVIOUS, DL_ALBUM) );
    printf("Previous Album: ");
    print_song_info( so_n );
    CU_ASSERT( DS_SUCCESS == next_song(&so_n, DT_PREVIOUS, DL_ARTIST) );
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
