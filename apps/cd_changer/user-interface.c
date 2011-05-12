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
#include <stdint.h>
#include <string.h>

#include <linked-list/linked-list.h>

#include "user-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MAX_UI_NODES    4

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static ll_list_t __ui_list;
static ll_node_t __nodes[MAX_UI_NODES];
static ui_impl_t *__current;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
ll_ir_t iterator( ll_node_t *node, volatile void *user_data );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See user-interface.h for details. */
bool ui_init( void )
{
    ll_init_list( &__ui_list );

    __current = NULL;

    return false;
}

/* See user-interface.h for details. */
bool ui_register( const ui_impl_t *impl )
{
    static uint32_t __next_available_ui = 0;

    if( MAX_UI_NODES <= __next_available_ui ) {
        return false;
    }

    if( (NULL == impl) ||
        (NULL == impl->name) ||
        (0 == strlen(impl->name)) ||
        (NULL == impl->ui_get_disc_info_fn) ||
        (NULL == impl->ui_process_command_fn) )
    {
        return false;
    }

    ll_init_node( &__nodes[__next_available_ui], (ui_impl_t*) impl );
    ll_append( &__ui_list, &__nodes[__next_available_ui] );

    __next_available_ui++;

    /* If there is no active user interface, set one now.
     * __current should always remain set. */
    if( NULL == __current ) {
        __current = (ui_impl_t*) impl;
    }

    return true;
}

/* See user-interface.h for details. */
bool ui_select( const char *name )
{
    ui_impl_t *old;

    old = __current;
    __current = NULL;

    ll_iterate( &__ui_list, iterator, NULL, (char*) name );

    if( NULL == __current ) {
        /* Not found, revert. */
        __current = old;
        return false;
    }

    return true;
}

/* See user-interface.h for details. */
void* ui_user_data_init( void )
{
    if( NULL != __current->ui_user_data_init_fn ) {
        return (*__current->ui_user_data_init_fn)();
    }

    return NULL;
}

/* See user-interface.h for details. */
void ui_user_data_destroy( void *user_data )
{
    if( NULL != __current->ui_user_data_destroy_fn ) {
        (*__current->ui_user_data_destroy_fn)( user_data );
    }
}

/* See user-interface.h for details. */
void ui_dir_map_release( const char **map )
{
    if( NULL != __current->ui_dir_map_release_fn ) {
        (*__current->ui_dir_map_release_fn)( map );
    }
}

/* See user-interface.h for details. */
void ui_get_disc_info( uint8_t *map, uint8_t *disc, uint8_t *track,
                       song_node_t **song, void *user_data )
{
    return (*__current->ui_get_disc_info_fn)( map, disc, track, song, user_data );
}

/* See user-interface.h for details. */
void ui_process_command( irp_state_t *device_status, irp_mode_t *device_mode,
                         const uint8_t disc_map, uint8_t *current_disc,
                         uint8_t *current_track, const ri_msg_t *msg,
                         song_node_t **song, void *user_data )
{
    (*__current->ui_process_command_fn)( device_status, device_mode, disc_map, current_disc,
                                         current_track, msg, song, user_data );
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to walk the interfaces & find the name match & set the __current
 *  pointer.
 */
ll_ir_t iterator( ll_node_t *node, volatile void *user_data )
{
    if( 0 == strcmp((char*) user_data, ((ui_impl_t*) node->data)->name) ) {
        __current = (ui_impl_t*) node->data;
        return LL_IR__STOP;
    }

    return LL_IR__CONTINUE;
}
