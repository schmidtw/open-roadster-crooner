#include <stdint.h>
#include <stdlib.h>

#include "database.h"
#include "indexer.h"
#include "next_song.h"

bt_ir_t index_generic( bt_node_t *node, void *user_data );


void index_root( bt_node_t * root )
{
    uint32_t indexer = 0;
    generic_node_t *root_node = (generic_node_t*)root->data;
    bt_iterate( &root_node->i.list.children, index_generic, NULL, &indexer);
    root_node->i.list.index_songs_stop = indexer - 1;
    bt_set_compare( &root_node->i.list.children, compare_indexed_general );
}

bt_ir_t index_generic( bt_node_t *node, void *user_data )
{
    uint32_t *indexer = (uint32_t *)user_data;
    generic_node_t *gn = (generic_node_t *)node->data;

    if( GNT_SONG == gn->type ) {
        gn->i.song_index = *indexer;
        (*indexer)++;
    } else {
        gn->i.list.index_songs_start = *indexer;
        bt_iterate(&(gn->i.list.children), index_generic, NULL, indexer);
        gn->i.list.index_songs_stop = *indexer - 1;
        if( GNT_ALBUM == gn->type ) {
            bt_set_compare( &gn->i.list.children, compare_indexed_song );
        } else {
            bt_set_compare( &gn->i.list.children, compare_indexed_general );
        }
    }
    return BT_IR__CONTINUE;
}
