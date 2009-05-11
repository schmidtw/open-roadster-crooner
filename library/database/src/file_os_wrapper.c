#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fatfs/ff.h>
#include "file_os_wrapper.h"

DIR cur_dir;

file_return_value open_directory( char * path )
{
    FRESULT rv;
    if( NULL == path ) {
        return FRV_INVALID_PARAMETER;
    }
    rv = f_opendir(&cur_dir, path);
    if( FR_OK != rv ) {
        return FRV_ERROR;
    }
    return FRV_RETURN_GOOD;
}

file_return_value get_next_element_in_directory( file_info_t * f_info )
{
    FILINFO file_info;
    FRESULT rv;
    if( NULL == f_info ) {
        return FRV_INVALID_PARAMETER;
    }
    
    while( 1 ) {
        rv = f_readdir(&cur_dir, &file_info);
        if( FR_OK != rv ) {
            return FRV_ERROR;
        }
        if( file_info.fname[0] == '\0' ) {
            return FRV_END_OF_ENTRIES;
        }
        /* The fatfs will have the null terminated character at the end of the
         * fname field.  So we don't have to do any manipulation to make sure
         * the string is properly terminated.  Also the fname is hardcoded to
         * 13 characters, which is large enough to handle the 8.3 plus NULL
         * format.
         */
        strcpy(f_info->short_filename, file_info.fname );
        f_info->is_dir = ( file_info.fattrib & AM_DIR );
        /* If the file attributes are anything but DIR, then we don't want
         * to pass this file entry back.  Otherwise we want to send the file
         * information back.
         */
        if( 0 == ( file_info.fattrib & ( AM_MASK ^ AM_DIR ) ) ) {
            break;
        }
    }
    return FRV_RETURN_GOOD;
}
