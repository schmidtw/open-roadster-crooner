#include <stdbool.h>
#include <stddef.h>
#include <media-flac/media-flac.h>
#include <media-interface/media-interface.h>

#include "mi_interface.h"

static bool mi_is_initialized = false;
static media_interface_t * mi_list = NULL;

bool mi_init( void )
{
    media_status_t rv;
    if( true == mi_is_initialized ) {
        return false;
    }
    
    mi_list = media_new();
    if( NULL == mi_list ) {
        printf("mi_list is NULL in mi_init\n");
        return false;
    }
    mi_is_initialized = true;
    
    rv = media_register_codec( mi_list, "flac", media_flac_command,
                               media_flac_play, media_flac_get_type,
                               media_flac_get_metadata );
    if( MI_RETURN_OK != rv ) {
        goto failure;
    }
    
    return true;
failure:
    mi_delete();
    return false;
}

media_status_t mi_get_information( const char *filename,
                                   media_metadata_t *metadata,
                                   media_command_fn_t *command_fn,
                                   media_play_fn_t *play_fn )
{
    if( false == mi_is_initialized ) {
        return MI_ERROR_PARAMETER;
    }
    return media_get_information( mi_list, filename, metadata,
                                  command_fn, play_fn );
}

/**
 * Used to delete registered codecs from the local list
 * 
 * @return MI_ERROR_PARAMETER
 *          MI_RETURN_OK
 */
media_status_t mi_delete( void )
{
    if( false == mi_is_initialized ) {
        return MI_ERROR_PARAMETER;
    }
    mi_is_initialized = false;
    return media_delete( mi_list );
}
