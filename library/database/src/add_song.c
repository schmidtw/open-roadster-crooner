#include <stdbool.h>

#include <linked-list/linked-list.h>

#include "add_song.h"
#include "database.h"
#include "generic.h"
#include "song.h"

static ll_ir_t scrubber(ll_node_t *node, volatile void *user_data);

song_node_t * add_song_to_root( generic_node_t * root,
        media_metadata_t * metadata,
        media_play_fn_t play_fn,
        char * file_location )
{
    generic_node_t * artist_n;
    generic_node_t * album_n;
    song_node_t * song_n;
    bool artist_created = false;
    bool album_created = false;
    
    if( NULL != metadata ) {
        artist_n = find_or_create_generic( root, generic_compare,
                (void*)metadata->artist, &artist_created );
        if( NULL != artist_n ) {
            album_n = find_or_create_generic( artist_n, generic_compare,
                    (void*)metadata->album, &album_created );
            if( NULL != album_n ) {
                bool song_created;
                song_create_t sc;
                sc.metadata = metadata;
                sc.play_fn = play_fn;
                sc.file_location = file_location;
                song_n = (song_node_t*)find_or_create_generic( album_n,
                        song_compare, (void*)&sc, &song_created);
                if( NULL != song_n ) {
                    return song_n;
                }
            }
        }
    }
    if( true == artist_created ) {
        ll_iterate( &artist_n->parent->children, scrubber, delete_generic, &artist_n->node );
    } else if( true == album_created ) {
        ll_iterate( &album_n->parent->children, scrubber, delete_generic, &album_n->node );
    }
    return NULL;
}

static ll_ir_t scrubber(ll_node_t *node, volatile void *user_data)
{
    if( node == (ll_node_t *)user_data ) {
        return LL_IR__DELETE_AND_STOP;
    }
    return LL_IR__CONTINUE;
}
