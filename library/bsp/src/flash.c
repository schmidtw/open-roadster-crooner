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

#include <string.h>

#include <avr32/io.h>

#include "flash.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See flash.h for details. */
size_t flash_get_user_page_size( void )
{
    return AVR32_FLASHC_USER_PAGE_SIZE;
}

/* See flash.h for details. */
bsp_status_t flash_user_page_read( void *out )
{
    volatile void *user_page;

    if( NULL == out ) {
        return BSP_ERROR_PARAMETER;
    }

    user_page = AVR32_FLASHC_USER_PAGE;

    memcpy( out, (void*) user_page, AVR32_FLASHC_USER_PAGE_SIZE );

    return BSP_RETURN_OK;
}

/* See flash.h for details. */
bsp_status_t flash_user_page_write( const void *in )
{
    if( NULL == in ) {
        return BSP_ERROR_PARAMETER;
    }

    /* Need to implement this with the magic work-around. */

#if 0
    return BSP_RETURN_OK;
#else
    return BSP_ERROR_UNSUPPORTED;
#endif
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
