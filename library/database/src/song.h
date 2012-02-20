#ifndef __SONG_H__
#define __SONG_H__

#include <media-interface/media-interface.h>
#include "database.h"

typedef struct {
    generic_node_types_t type;
    media_metadata_t *metadata;
    media_play_fn_t play_fn;
    char * file_location;
} song_create_t;

#endif /* __SONG_H__ */
