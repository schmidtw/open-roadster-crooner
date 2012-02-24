#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#include <stdbool.h>

/**
 * Remove the last directory from the string
 *
 * example: string * = /blue/black/green
 *  output: true, /blue/black
 *
 * @param string pointer to the string to manipulate
 * @param length The length of the string being pointed to
 *
 * @return true on success, false otherwise.  On success the length
 *         is updated to reflect the new string size
 */
bool remove_last_dir_name( char * string, size_t *length_of_src );

/**
 * Will place the src string plus a '/' onto
 * the end of the dest string.
 *
 * ex: dest * = /blue/black
 *     dest_length * =  11
 *      src * = green
 * result:
 *     dest * = /blue/black/green
 *     dest_length * = 17
 *
 * @param dest a buffer large enough to support max_characters+1 more
 *             characters
 * @param dest_size the current length of the string pointed to by the dest
 * @param src buffer which contains the new string
 *
 * @return true on success, false otherwise
 */
bool append_to_path( char * dest, size_t *dest_length, const char * src );

#endif /* __FILE_HELPER_H__ */
