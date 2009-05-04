/*
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

#include <stdlib.h>
#include <linked-list/linked-list.h>
#include "database.h"
#include "group.h"
#include "artist.h"
#include "w_malloc.h"

ll_node_t * get_new_group_and_node( const char * name )
{
    group_node_t * gn;
    
    if( NULL == name ) {
        return NULL;
    }
    
    gn = (group_node_t *) w_malloc( sizeof(group_node_t) );
    if( gn == NULL ) {
        return NULL;
    }
    
    ll_init_node( &(gn->node), gn );
    strncpy( gn->name, name, sizeof(char) * MAX_GROUP_NAME);
    gn->name[MAX_GROUP_NAME] = '\0';
    ll_init_list(&(gn->artists));
    
    return &(gn->node);
}

void delete_group(ll_node_t *node, volatile void *user_data)
{
    group_node_t *gn;
    
    gn = (group_node_t *)node->data;
    ll_delete_list(&gn->artists, delete_artist, NULL);
    free(gn);
}
