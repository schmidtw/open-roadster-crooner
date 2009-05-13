#include "add_song.h"
#include "database.h"
#include "artist.h"
#include "album.h"
#include "song.h"

song_node_t * add_song_to_group( group_node_t * group,
        const char * artist, const char * album,
        const char * song, const uint8_t track_number,
        media_command_fn_t *command_fn,
        media_play_fn_t *play_fn,
        const char * file_location )
{
    artist_node_t * ar_n;
    album_node_t * al_n;
    song_node_t * so_n = NULL;
    
    ar_n = find_or_create_artist( group, artist );
    if( NULL != ar_n ) {
        al_n = find_or_create_album( ar_n, album );
        if( NULL != al_n ) {
            so_n = find_or_create_song( al_n, song, track_number, command_fn, play_fn, file_location );
        }
    }
    return so_n;
}
