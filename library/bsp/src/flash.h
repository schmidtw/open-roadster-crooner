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
#ifndef __FLASH_H__
#define __FLASH_H__

#include <stddef.h>

#include "bsp_errors.h"

/**
 *  Used to get the user page size in bytes.
 *
 *  @return the size of the user page in bytes.
 */
size_t flash_get_user_page_size( void );

/**
 *  Used to read the user page into a user provided
 *  buffer.
 *
 *  @param out the buffer to read into
 *
 *  @return Status
 *      @retval BSP_RETURN_OK               Success
 *      @retval BSP_ERROR_PARAMETER         Invalid parameter
 */
bsp_status_t flash_user_page_read( void *out );

/**
 *  Used to write the value specified into the user page.
 *
 *  @param in the buffer to write to the user page
 *
 *  @return Status
 *      @retval BSP_RETURN_OK               Success
 *      @retval BSP_ERROR_UNSUPPORTED       Feature not supported
 *      @retval BSP_ERROR_PARAMETER         Invalid parameter
 */
bsp_status_t flash_user_page_write( const void *in );
#endif
