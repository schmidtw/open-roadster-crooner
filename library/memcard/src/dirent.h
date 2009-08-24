#ifndef __DIRENT_H__
#define __DIRENT_H__
#define _DIRENT_H_

#include <sys/types.h>

#define DT_NAME_MAX    12

#define DT_DIR          0x01
#define DT_READ_ONLY    0x02

struct dirent {
    ino_t d_ino;
    char d_name[DT_NAME_MAX+1];
    size_t d_size;
    unsigned char d_attr;
};

#ifndef _FATFS
typedef void DIR;
#endif

/* readdir_r is not supported */

/**
 *  Used to close a directory stream
 *
 *  @param dirp the directory pointer to close
 *
 *  @return 0 on success, -1 on error with errno set
 */
int closedir( DIR *dirp );

/**
 *  Used to open a directory stream
 *
 *  @param dirname the name of the directory to open
 *
 *  @return pointer to stream on success, NULL on error
 *          with errno set
 */
DIR* opendir( const char *dirname );

/**
 *  Used to read from a directory stream
 *
 *  @note _user_provided is non-standard to allow for reduced memory
 *        churn.
 *
 *  @param dirp the directory pointer to read from
 *  @param _user_provided the user provided dirent struct for
 *         data population and returning if successful
 *
 *  @return next data entry information on success, NULL on end of
 *          stream with errno unchanged, and NULL with errno set on
 *          error
 */
struct dirent* readdir( DIR *dirp, struct dirent *_user_provided );

/**
 *  Used to rewind a directory stream
 *
 *  @param dirp the directory pointer to rewind
 */
void rewinddir( DIR *dirp );

/**
 *  Used to seek to a directory stream entry
 *
 *  @param dirp the directory pointer to close
 *  @param loc the index of the desired entry
 */
void seekdir( DIR *dirp, long loc );

/**
 *  Used to report the current location in directory stream
 *
 *  @param dirp the directory pointer to close
 *
 *  @return current location on success, -1 on error with errno set
 */
long telldir( DIR *dirp );

#endif
