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
#ifndef __FILE_STREAM_H__
#define __FILE_STREAM_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void *(*fstream_malloc_fct)( size_t size );
typedef void (*fstream_free_fct)( void *ptr );

/**
 *  Starts the file-stream 'server'.
 *
 *  @param max_big_buffer_size the size of the largest buffer returned in bytes
 *  @param small_buffer_size the size in bytes of the individual small storage
 *                           buffers to keep around
 *  @param small_buffer_count the maximum number of small buffers to keep around
 */
bool fstream_init( const uint32_t priority,
                   fstream_malloc_fct malloc_fn,
                   fstream_free_fct free_fn );

/**
 *  Takes an open file handle and starts buffering & streaming
 *  from the beginning of the file.
 *
 *  @param filename the name of the file to stream
 *
 *  @return true on success, false otherwise
 */
bool fstream_open( const char *filename );

/**
 *  Used to get the next block of bytes from a file.
 *
 *  @param wanted the number of bytes desired
 *  @param got the number of bytes that are returned - it should be
 *         different only at the end of the file
 *
 *  @return the pointer to the buffer, or NULL on error or if no file
 *          is open
 */
void* fstream_get_buffer( const size_t wanted, size_t *got );

/**
 *  Releases the active buffer & return any remaining data back to
 *  the file stream server for later retrieval.
 *
 *  @param consumed the number of bytes out of the buffer that were consumed
 */
void fstream_release_buffer( const size_t consumed );

/**
 *  Skips a defined number of bytes from the file stream.
 *
 *  @param skip the number of bytes to skip
 */
void fstream_skip( const size_t skip );

/**
 *  Closes & discards any remaining file data.
 */
void fstream_close( void );

/**
 *  Destroys the stream server.
 */
void fstream_destroy( void );

/**
 *  Used to get the size of the currently open file in bytes.
 *
 *  @return the size of the file in bytes
 */
uint32_t fstream_get_filesize( void );
#endif
