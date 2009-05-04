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
#ifndef __MEDIA_FLAC_H__
#define __MEDIA_FLAC_H__

#include <media-interface/media-interface.h>

/** See media-interface.h for details. */
media_status_t media_flac_command( const media_command_t cmd );

/** See media-interface.h for details. */
media_status_t media_flac_play( const char *filename,
                                media_suspend_fn_t suspend,
                                media_resume_fn_t resume );

/** See media-interface.h for details. */
bool media_flac_get_type( const char *filename );

/** See media-interface.h for details. */
media_status_t media_flac_get_metadata( const char *filename,
                                        media_metadata_t *metadata );
#endif
