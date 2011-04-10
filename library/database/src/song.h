#ifndef __SONG_H__
#define __SONG_H__

#include <media-interface/media-interface.h>

typedef struct {
    media_metadata_t *metadata;
    media_play_fn_t play_fn;
    char * file_location;
} song_create_t;

#endif /* __SONG_H__ */
