#include <stdint.h>
#include <stdlib.h>

#include "database.h"
#include "indexer.h"
#include "next_song.h"


typedef struct {
    uint32_t song_index;
    uint32_t identifier_in_list;
} indexer_t;

bt_ir_t index_generic( bt_node_t *node, void *user_data );


void index_root( bt_node_t * root )
{
    indexer_t indexer = {.song_index=0, .identifier_in_list=1};
    generic_node_t *root_node = (generic_node_t*)root->data;
    bt_iterate( &root_node->children, index_generic, NULL, &indexer);
    root_node->d.list.index_songs_stop = indexer.song_index -1;
    bt_set_compare( &root_node->children, compare_indexed_general );
}

bt_ir_t index_generic( bt_node_t *node, void *user_data )
{
    indexer_t *indexer = (indexer_t *)user_data;
    generic_node_t *gn = (generic_node_t *)node->data;
    indexer_t indexed;
    indexed.song_index = indexer->song_index;
    indexed.identifier_in_list = 1;

    if( 0 == indexer->identifier_in_list ) {
        (indexer->identifier_in_list)++;
    }
    gn->d.list.index = indexer->identifier_in_list;
    (indexer->identifier_in_list)++;

    if( GNT_SONG == gn->type ) {
        ((song_node_t*)gn)->index_songs_value = indexer->song_index;
        indexer->song_index++;
    } else {
        gn->d.list.index_songs_start = indexer->song_index;
        bt_iterate(&(gn->children), index_generic, NULL, &indexed);
        indexer->song_index = indexed.song_index;
        gn->d.list.index_songs_stop = indexer->song_index - 1;
        if( GNT_ALBUM == gn->type ) {
            bt_set_compare( &gn->children, compare_indexed_song );
        } else {
            bt_set_compare( &gn->children, compare_indexed_general );
        }
    }
    return BT_IR__CONTINUE;
}
