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


#define _D1(...)

#if DEBUG > 0
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

bool remove_last_dir_name( char * string, size_t *length )
{
    char *i_src;
    if(    ( NULL == string )
        || ( NULL == length ) ) {
        return false;
    }
    _D1("%s: %s, %u\n", "remove_last_dir_name", string, *length);
    // We never worry about a trailing slash so subtract 1
    *length -= 2;
    i_src = string + *length;

    while( i_src >= string ) {
        if( '/' == *i_src ) {
            if( i_src == string ) {
                *(i_src + 1) = '\0';
                (*length)++;
            } else {
                *(i_src) = '\0';
            }
            _D1("%s: done %s, %u\n", "remove_last_dir_name", string, *length);
            return true;
        }
        i_src--;
        (*length)--;
    }
    return false;
}

bool append_to_path( char * dest, size_t *dest_length, const char * src )
{
    char *i_dst;
    bool add_trailing_slash = true;
    if(    (NULL == dest)
        || (NULL == src)
        || (NULL == dest_length) ) {
        return false;
    }
    _D1("%s: %s, %u -- toappend %s\n", "append_to_path", dest, *dest_length, src);

    i_dst = dest + *dest_length - 1;
    if( '/' == *i_dst++ ) {
        add_trailing_slash = false;
    }
    if( true == add_trailing_slash ) {
        *i_dst++ = '/';
        (*dest_length)++;
    }
    strcpy( i_dst, src );
    *dest_length += strlen(src);

    _D1("%s: done %s, %u\n", "append_to_path", dest, *dest_length);
    return true;
}
