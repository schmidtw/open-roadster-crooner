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
#include <stdint.h>
#include <string.h>

#include "file_helper.h"

bool get_last_dir_name( char * dest, char * src )
{
    char *i_src = src;
    char *i_dst = dest;
    char *last_slash = NULL;

    if(    (NULL == dest)
        || (NULL == src) ) {
        return false;
    }

    while( '\0' != *i_src ) {
        if(    ( '/' == *i_src )
            && ( '\0' != *(i_src+1) ) )
        {
            last_slash = i_src;
        }
        i_src++;
    }
    if( NULL == last_slash ) {
        return false;
    }

    i_src = last_slash + 1;
    if( last_slash == src ) {
        last_slash++;
    }
    while(    ( '\0' != *i_src )
           && ( '/' != *i_src ) )
    {
        *i_dst++ = *i_src++;
    }
    *last_slash = '\0';
    *i_dst = '\0';
    return true;
}


void append_to_path( char * dest, const char * src )
{
    char *i_dst;
    bool add_trailing_slash = true;
    if(    (NULL == dest)
        || (NULL == src) ) {
        return;
    }

    i_dst = dest;
    while( '\0' != *i_dst ) {
        add_trailing_slash = ( '/' != *i_dst );
        i_dst++;
    }
    if( true == add_trailing_slash ) {
        *i_dst++ = '/';
    }
    strcpy( i_dst, src );
}
