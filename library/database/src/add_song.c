#include <stdbool.h>
#include "add_song.h"
#include "database.h"
#include "artist.h"
#include "album.h"
#include "song.h"

ll_ir_t scrubber(ll_node_t *node, volatile void *user_data);

song_node_t * add_song_to_group( group_node_t * group,
        const char * artist, const char * album,
        const char * song, const uint16_t track_number,
        const double track_gain, const double track_peak,
        const double album_gain, const double album_peak,
        media_command_fn_t command_fn,
        media_play_fn_t play_fn,
        const char * file_location )
{
    artist_node_t * ar_n;
    album_node_t * al_n;
    song_node_t * so_n = NULL;
    bool artist_created = false;
    bool album_created = false;
    
    ar_n = find_or_create_artist( group, artist, &artist_created );
    if( NULL == ar_n ) {
        goto failure;
    } else {
        al_n = find_or_create_album( ar_n, album, &album_created );
        if( NULL == al_n ) {
            goto failure;
        } else {
            so_n = find_or_create_song( al_n, song, track_number, album_gain, album_peak,
                                        track_gain, track_peak, command_fn, play_fn,
                                        file_location );
            if( NULL == so_n ) {
                goto failure;
            }
        } 
    }
    return so_n;
failure:
    if( true == artist_created ) {
        ll_iterate( &ar_n->group->artists, scrubber, delete_artist, &ar_n->node );
    } else if( true == album_created ) {
        ll_iterate( &al_n->artist->albums, scrubber, delete_album, &al_n->node );
    }
    return NULL;
}

ll_ir_t scrubber(ll_node_t *node, volatile void *user_data)
{
    if( node == (ll_node_t *)user_data ) {
        return LL_IR__DELETE_AND_STOP;
    }
    return LL_IR__CONTINUE;
}
