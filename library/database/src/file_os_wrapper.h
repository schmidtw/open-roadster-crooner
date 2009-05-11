#ifndef __FILE_OS_WRAPPER_H__
#define __FILE_OS_WRAPPER_H__

#include <stdbool.h>

#define MAX_SHORT_FILENAME        12
#define MAX_SHORT_FILENAME_W_NULL ( MAX_SHORT_FILENAME + 1 )

typedef enum {
    FRV_RETURN_GOOD,
    FRV_INVALID_PARAMETER,
    FRV_END_OF_ENTRIES,
    FRV_ERROR
} file_return_value;

typedef struct {
    char short_filename[MAX_SHORT_FILENAME_W_NULL];
    bool is_dir;
} file_info_t;

file_return_value get_next_element_in_directory( file_info_t * f_info );
file_return_value open_directory( char * path );

#endif /* __FILE_OS_WRAPPER_H__ */
