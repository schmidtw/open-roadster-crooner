/*
 * Copyright (c) 2009  Weston Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdbool.h>
#include <stdlib.h>
#include <linked-list/linked-list.h>

#include "media-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    const char *name;
    ll_node_t node;

    media_command_fn_t command;
    media_play_fn_t play;
    media_get_type_fn_t get_type;
    media_get_metadata_fn_t get_metadata;
} media_type_functions_t;

typedef struct {
    const char *filename;
    media_type_functions_t *node;
} media_iterator_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static ll_ir_t __iterator( ll_node_t *node, volatile void *user_data );
static void __deleter( ll_node_t *node, volatile void *user_data );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See media-interface.h for details. */
media_interface_t *media_new( void )
{
    ll_list_t *codec_list;

    codec_list = (ll_list_t*) malloc( sizeof(ll_list_t) );
    if( NULL != codec_list ) {
        ll_init_list( codec_list );
    }

    return (media_interface_t *) codec_list;
}

/** See media-interface.h for details. */
media_status_t media_register_codec( media_interface_t *interface,
                                     const char *name,
                                     media_command_fn_t command,
                                     media_play_fn_t play,
                                     media_get_type_fn_t get_type,
                                     media_get_metadata_fn_t get_metadata )
{
    media_type_functions_t *node;
    ll_list_t *codec_list;

    codec_list = (ll_list_t *) interface;

    if( (NULL == codec_list) || (NULL == name) || (NULL == command) ||
        (NULL == play) || (NULL == get_type) || (NULL == get_metadata) )
    {
        return MI_ERROR_PARAMETER;
    }

    node = (media_type_functions_t*) malloc( sizeof(media_type_functions_t) );
    if( NULL == node ) {
        return MI_ERROR_OUT_OF_MEMORY;
    }

    ll_init_node( &node->node, node );
    node->name = name;
    node->command = command;
    node->play = play;
    node->get_type = get_type;
    node->get_metadata = get_metadata;

    ll_append( codec_list, &node->node );

    return MI_RETURN_OK;
}

/** See media-interface.h for details. */
media_status_t media_get_information( media_interface_t *interface,
                                      const char *filename,
                                      media_metadata_t *metadata,
                                      media_command_fn_t *command_fn,
                                      media_play_fn_t *play_fn )
{
    media_iterator_t info;
    ll_list_t *codec_list;

    codec_list = (ll_list_t *) interface;

    if( (NULL == codec_list) || (NULL == filename) ||
        ((NULL == metadata) && (NULL == command_fn) && (NULL == play_fn)) )
    {
        return MI_ERROR_PARAMETER;
    }

    info.filename = filename;
    info.node = NULL;
    ll_iterate( codec_list, &__iterator, NULL, &info );

    if( NULL == info.node ) {
        return MI_ERROR_NOT_SUPPORTED;
    }

    if( NULL != command_fn ) {
        *command_fn = info.node->command;
    }

    if( NULL != play_fn ) {
        *play_fn = info.node->play;
    }

    if( NULL != metadata ) {
        media_status_t status;

        status = (*info.node->get_metadata)( filename, metadata );
        if( MI_RETURN_OK != status ) {
            return status;
        }
    }

    return MI_RETURN_OK;
}

/** See media-interface.h for details. */
media_status_t media_delete( media_interface_t *interface )
{
    ll_list_t *codec_list;

    codec_list = (ll_list_t *) interface;

    if( NULL == codec_list ) {
        return MI_ERROR_PARAMETER;
    }

    ll_delete_list( codec_list, __deleter, NULL );

    free( codec_list );
    return MI_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static ll_ir_t __iterator( ll_node_t *node, volatile void *user_data )
{
    media_type_functions_t *type_node;
    media_iterator_t *info;
    
    type_node = (media_type_functions_t *) node->data;
    info = (media_iterator_t *) user_data;

    if( true == (*type_node->get_type)(info->filename) ) {
        info->node = type_node;
        return LL_IR__STOP;
    }

    return LL_IR__CONTINUE;
}

static void __deleter( ll_node_t *node, volatile void *user_data )
{
    free( node->data );
}
