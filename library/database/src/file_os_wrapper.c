#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <memcard/dirent.h>
#include "file_os_wrapper.h"

DIR *cur_dir = NULL;

file_return_value open_directory( char * path )
{
    if( NULL == path ) {
        return FRV_INVALID_PARAMETER;
    }

    if( NULL != cur_dir ) {
        closedir( cur_dir );
        cur_dir = NULL;
    }

    cur_dir = opendir( path );

    if( NULL == cur_dir ) {
        return FRV_ERROR;
    }

    return FRV_RETURN_GOOD;
}

file_return_value get_next_element_in_directory( file_info_t * f_info )
{
    struct dirent file_info;
    struct dirent *out;
    int old_errno;

    if( NULL == f_info ) {
        return FRV_INVALID_PARAMETER;
    }

    old_errno = errno;
    out = readdir( cur_dir, &file_info );
    if( (NULL == out) && (old_errno != errno) ) {
        closedir( cur_dir );
        cur_dir = NULL;
        return FRV_ERROR;
    }

    if( NULL == out ) {
        closedir( cur_dir );
        cur_dir = NULL;
        return FRV_END_OF_ENTRIES;
    }

    /* The fatfs will have the null terminated character at the end of the
     * fname field.  So we don't have to do any manipulation to make sure
     * the string is properly terminated.  Also the fname is hardcoded to
     * 13 characters, which is large enough to handle the 8.3 plus NULL
     * format.
     */
    strcpy(f_info->short_filename, file_info.d_name );
    f_info->is_dir = ( file_info.d_attr & DT_DIR );
    /* If the file attributes are anything but DIR, then we don't want
     * to pass this file entry back.  Otherwise we want to send the file
     * information back.
     */
    return FRV_RETURN_GOOD;
}
