#include <stdint.h>
#include <stdlib.h>

#include "database.h"
#include "indexer.h"


typedef struct {
    uint32_t song_index;
    uint32_t identifier_in_list;
} indexer_t;

ll_ir_t index_generic( ll_node_t *node, volatile void *user_data );


void index_groups( ll_list_t * groups )
{
    indexer_t indexer = {.song_index=0, .identifier_in_list=1};
    ll_iterate(groups, index_generic, NULL, &indexer);
}

ll_ir_t index_generic( ll_node_t *node, volatile void *user_data )
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
        ll_iterate(&(gn->children), index_generic, NULL, &indexed);
        indexer->song_index = indexed.song_index;
        gn->d.list.index_songs_stop = indexer->song_index - 1;
    }
    return LL_IR__CONTINUE;
}
