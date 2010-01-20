/*
 * dfu-programmer
 *
 * $Id: intel_hex.h 48 2007-03-28 07:35:45Z schmidtw $
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

#ifndef __INTEL_HEX_H__
#define __INTEL_HEX_H__

#include <stdint.h>

/**
 *  Used to read in the specified number of bytes from the data stream
 *
 *  @param data_stream the data stream to read from
 *  @param bytes the number of bytes to read
 *  @param data the pre-allocated buffer to read data into
 *
 *  @return the number of bytes read, 0 for no more data, < 0 on error
 */
typedef int32_t (*fgets_fn_t)( void *data_stream, const size_t bytes, char *data );

typedef int32_t (*fgetc_fn_t)( void *data_stream );

typedef struct {
    fgets_fn_t fgets;
    fgetc_fn_t fgetc;
    void *data_stream;
} intel_hex_data_reader_t;

/**
 *  Used to read in a file in intel hex format and return a chunck of
 *  memory containing the memory image described in the file.
 *
 *  @param max_size the maximum size of the memory image in bytes
 *  @param usage[out] the amount of the available memory image used
 *
 *  @return an array of int16_t's where the values 0-255 are valid memory
 *          values, and anything else indicates an unused memory location,
 *          NULL on anything other than a success
 */
int16_t *intel_hex_to_buffer( intel_hex_data_reader_t *readers, int max_size, int *usage );

#endif
