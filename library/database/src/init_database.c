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

#include <stdbool.h>
#include <stdint.h>
#include <linked-list/linked-list.h>
#include "database.h"
#include "internal_database.h"
#include "group.h"

root_database_node_t rdn;

/* See database.h for information */
bool init_database( const openDir_fct openDir,
                    const getNext_fct openNext,
                    const fclose_fct file_close,
                    const fopen_fct file_open
                    /* function for metadata */
                    )
{
    rdn.openDir = openDir;
    rdn.openNext = openNext;
    rdn.fclose = file_close;
    rdn.fopen = file_open;
    
    rdn.initialized = true;
    
    return true;
}
