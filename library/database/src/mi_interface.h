#ifndef __MI_INTERFACE_H__
#define __MI_INTERFACE_H__

#include <stdbool.h>
#include <media-interface/media-interface.h>

/**
 * Sets up the linked list of supported media formats.
 */
bool mi_init( void );

/**
 * Finds a supported media format.  Populates the
 * metadata, command_fn, play_fn with the appropriate
 * function pointer/data.
 * 
 * @return MI_RETURN_OK when the format is found
 *         MI_ERROR_PARAMETER otherwise
 */
media_status_t mi_get_information( const char *filename,
                                   media_metadata_t *metadata,
                                   media_command_fn_t *command_fn,
                                   media_play_fn_t *play_fn );

/**
 * Used to delete registered codecs from the local list
 * 
 * @return MI_ERROR_PARAMETER
 *          MI_RETURN_OK
 */
media_status_t mi_delete( void );

#endif
