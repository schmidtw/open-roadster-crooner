#include <stdbool.h>

#include <binary-tree-avl/binary-tree-avl.h>

#include "add_song.h"
#include "database.h"
#include "generic.h"
#include "song.h"

song_node_t * add_song_to_root( generic_node_t * root,
        media_metadata_t * metadata,
        media_play_fn_t play_fn,
        char * file_location )
{
    generic_node_t * artist_n;
    generic_node_t * album_n;
    generic_holder_t holder;
    song_node_t * song_n;
    bool artist_created = false;
    bool album_created = false;
    
    if( NULL != metadata ) {
        holder.type = GNT_GENERIC_CREATE_NODE;
        holder.string = metadata->artist;
        artist_n = find_or_create_generic( root,
                (void*)&holder, &artist_created );
        if( NULL != artist_n ) {
            holder.string = metadata->album;
            album_n = find_or_create_generic( artist_n,
                    (void*)&holder, &album_created );
            if( NULL != album_n ) {
                bool song_created;
                song_create_t sc;
                sc.type = GNT_SONG_CREATE_NODE;
                sc.metadata = metadata;
                sc.play_fn = play_fn;
                sc.file_location = file_location;
                song_n = (song_node_t*)find_or_create_generic( album_n,
                        (void*)&sc, &song_created);
                if( NULL != song_n ) {
                    return song_n;
                }
            }
        }
    }
    if( true == artist_created ) {
        bt_remove( &artist_n->parent->children, (void*)artist_n, delete_generic, NULL );
    } else if( true == album_created ) {
        bt_remove( &album_n->parent->children, (void*)album_n, delete_generic, NULL );
    }

    return NULL;
}
