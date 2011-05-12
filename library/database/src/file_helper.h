#ifndef __FILE_HELPER_H__
#define __FILE_HELPER_H__

#include <stdbool.h>

/**
 * Will remove the last directory from the src and place the
 * string onto the dest ptr.  The dest buff must be able to
 * support short name filesize of 8 characters plus the
 * terminating character.
 *
 * ex: src * = /blue/black/green
 * result:
 *    dest * = green
 *     src * = /blue/black/
 */
bool get_last_dir_name( char * dest, char * src );

/**
 * Will place the src string plus a '/' onto
 * the end of the dest string.
 *
 * ex: dest * = /blue/black
 *      src * = green
 * result:
 *     dest * = /blue/black/green
 *
 * @param dest a buffer large enough to support max_characters+1 more
 *             characters
 * @param src buffer which contains the new string
 */
void append_to_path( char * dest, const char * src );

#endif /* __FILE_HELPER_H__ */
