#include <stdbool.h>
#include <stddef.h>
#include <media-flac/media-flac.h>
#include <media-interface/media-interface.h>

#include "mi_interface.h"

static media_interface_t * mi_list = NULL;

bool mi_init( media_interface_t *mi )
{
    mi_list = mi;
    return true;
}

media_status_t mi_get_information( const char *filename,
                                   media_metadata_t *metadata,
                                   media_command_fn_t *command_fn,
                                   media_play_fn_t *play_fn )
{
    return media_get_information( mi_list, filename, metadata,
                                  command_fn, play_fn );
}
