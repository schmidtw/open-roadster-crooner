#include <stdint.h>
#include <stdlib.h>

#include "database.h"
#include "indexer.h"
#include "next_song.h"

typedef struct {
    uint32_t *song_index;
    uint32_t this_index;
} __indexer_t;

bt_ir_t __index_generic( bt_node_t *node, void *user_data );

void index_root( bt_node_t * root )
{
    uint32_t song_index = 0;
    __indexer_t indexer = {.song_index = &song_index, .this_index = 1};

    generic_node_t *root_node = (generic_node_t*)root->data;

    root_node->index = 1;

    bt_iterate( &root_node->list.children, __index_generic, NULL, &indexer);
    root_node->list.index_songs_stop = song_index - 1;
    bt_set_compare( &root_node->list.children, compare_indexed_general );
}

bt_ir_t __index_generic( bt_node_t *node, void *user_data )
{
    __indexer_t *indexer = (__indexer_t *)user_data;
    generic_node_t *gn = (generic_node_t *)node->data;

    if( GNT_SONG == gn->type ) {
        gn->index = *(indexer->song_index);
        (*(indexer->song_index))++;
    } else {
        __indexer_t children_indexer = {.song_index = indexer->song_index,
                                        .this_index = 1};

        gn->index = indexer->this_index++;
        gn->list.index_songs_start = *(indexer->song_index);
        bt_iterate(&(gn->list.children), __index_generic, NULL, &children_indexer);
        gn->list.index_songs_stop = *(indexer->song_index) - 1;
        if( GNT_ALBUM == gn->type ) {
            bt_set_compare( &gn->list.children, compare_indexed_song );
        } else {
            bt_set_compare( &gn->list.children, compare_indexed_general );
        }
    }
    return BT_IR__CONTINUE;
}
