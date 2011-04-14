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
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/reent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <freertos/semphr.h>

#include <newlib/reent-file-glue.h>

#include "fatfs/ff.h"
#include "memcard-private.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xSemaphoreHandle __ff_mutex;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static FIL* __get_file( struct _reent *reent, int *fd );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void glue_init( void )
{
    vSemaphoreCreateBinary( __ff_mutex );
}

/*----------------------------------------------------------------------------*/
/*                        FF Synchronization Functions                        */
/*----------------------------------------------------------------------------*/

/**
 *  FatFS mount creation callback for synchronizing.
 *
 *  @param vol the volume being mounted
 *  @param sobj the sychronization object for the volume
 *
 *  @return TRUE on success, FALSE otherwise
 */
BOOL ff_cre_syncobj( BYTE vol, _SYNC_t *sobj )
{
    *sobj = mc_get_magic_insert_number();
    return TRUE;
}


/**
 *  FatFS unmount callback for synchronizing.
 *
 *  @param sobj the sychronization object
 *
 *  @return TRUE on success, FALSE otherwise
 */
BOOL ff_del_syncobj( _SYNC_t sobj )
{
    return TRUE;
}

/**
 *  FatFS synchronizing 'lock' call.
 *
 *  @param sobj the synchronization object
 *
 *  @return TRUE granted access, FALSE otherwise
 */
BOOL ff_req_grant( _SYNC_t sobj )
{
    xSemaphoreTake( __ff_mutex, portMAX_DELAY );
    if( sobj != mc_get_magic_insert_number() ) {
        xSemaphoreGive( __ff_mutex );
        return FALSE;
    }

    return TRUE;
}

/**
 *  FatFS synchronizing 'unlock' call.
 *
 *  @param sobj the synchronization object
 */
void ff_rel_grant( _SYNC_t sobj )
{
    xSemaphoreGive( __ff_mutex );
}

/*----------------------------------------------------------------------------*/
/*                            Newlib <=> FatFS Glue                           */
/*----------------------------------------------------------------------------*/
int _file_fstat_r( struct _reent *reent, int fd, struct stat *st )
{
    FIL *file;

    file = __get_file( reent, &fd );
    if( NULL == file ) {
        return -1;
    }

    /* Fill in the structure with all we know. */
    st->st_size = file->fsize;
    st->st_mode = S_IFREG | S_IFBLK;
    st->st_blksize = 512;

    return 0;
}

int _file_write_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    FIL *file;
    UINT wc;

    file = __get_file( reent, &fd );
    if( NULL == file ) {
        return -1;
    }

    switch( f_write(file, buf, len, &wc) ) {
        case FR_OK:
            return wc;

        case FR_DENIED:
        case FR_NOT_READY:
        case FR_INVALID_OBJECT:
        default:
            break;
    }

    reent->_errno = EBADF;
    return -1;
}

int _file_read_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    FIL *file;
    UINT rc;

    file = __get_file( reent, &fd );
    if( NULL == file ) {
        return -1;
    }

    switch( f_read(file, buf, len, &rc) ) {
        case FR_OK:
            return rc;

        case FR_DENIED:
        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_READY:
        case FR_INVALID_OBJECT:
        default:
            break;
    }

    reent->_errno = EBADF;
    return -1;
}

off_t _file_lseek_r( struct _reent *reent, int fd, off_t offset, int whence )
{
    FIL *file;
    DWORD goal;

    file = __get_file( reent, &fd );
    if( NULL == file ) {
        return -1;
    }

    if( SEEK_CUR == whence ) {
        goal = file->fptr + offset;
    } else if( SEEK_SET == whence ) {
        goal = offset;
    } else if( SEEK_END == whence ) {
        goal = file->fsize + offset;
    } else {
        reent->_errno = EINVAL;
        return -1;
    }

    if( goal < 0 ) {
        reent->_errno = EINVAL;
        return -1;
    }

    switch( f_lseek(file, goal) ) {
        case FR_OK:
            return goal;

        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_READY:
        case FR_INVALID_OBJECT:
        default:
            break;
    }

    reent->_errno = EBADF;
    return -1;
}

int _file_close_r( struct _reent *reent, int fd )
{
    struct reent_glue *r = (struct reent_glue*) reent;
    FIL *file;

    file = __get_file( reent, &fd );
    if( NULL == file ) {
        return -1;
    }

    r->active[fd] = false;

    switch( f_close(file) ) {
        case FR_OK:
            return 0;

        case FR_INVALID_OBJECT:
            reent->_errno = EBADF;
            break;

        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_READY:
        default:
            reent->_errno = EACCES;
            break;
    }

    return -1;
}

int _file_isatty_r( struct _reent *reent, int fd )
{
    struct reent_glue *r = (struct reent_glue*) reent;

    fd -= 3;

    reent->_errno = EBADF;

    if( REENT_GLUE_MAGIC == r->magic ) {
        /* Is our fd in range? */
        if( (0 <= fd) && (fd < REENT_GLUE_MAX_FILES) ) {
            /* Is our fd active? */
            if( 0 != r->active[fd] ) {
                reent->_errno = EINVAL;
            }
        }
    }

    return 0;
}

int _open_r( struct _reent *reent, const char *name, int flags, int mode )
{
    struct reent_glue *r = (struct reent_glue*) reent;
    int fd;
    FIL *file;
    BYTE mode_flags;

    if( REENT_GLUE_MAGIC != r->magic ) {
        reent->_errno = ENFILE;
        return -1;
    }

    for( fd = 0; fd < REENT_GLUE_MAX_FILES; fd++ ) {
        if( false == r->active[fd] ) {
            /* We found an empty fd. */
            break;
        }
    }

    if( REENT_GLUE_MAX_FILES == fd ) {
        reent->_errno = ENFILE;
        return -1;
    }

    if( NULL == r->file[fd] ) {
        r->file[fd] = (unsigned char *) malloc( sizeof(FF_SLOT) );
        if( NULL == r->file[fd] ) {
            reent->_errno = ENOMEM;
            return -1;
        }
    }

    file = (FIL *) r->file[fd];

    /* We must have these. */
    if( O_RDONLY == (O_ACCMODE & flags) ) {
        mode_flags = FA_READ;
    } else if( O_WRONLY == (O_ACCMODE & flags) ) {
        mode_flags = FA_WRITE;
    } else if( O_RDWR == (O_ACCMODE & flags) ) {
        mode_flags = FA_READ | FA_WRITE;
        if( O_APPEND == (O_APPEND & flags) ) {
            /* We can't support reading from the start & writing to the end. */
            reent->_errno = EINVAL;
            return -1;
        }
    } else {
        reent->_errno = EINVAL;
        return -1;
    }

    if( O_CREAT == (O_CREAT & flags) ) {
        mode_flags |= FA_OPEN_ALWAYS;
    } else {
        mode_flags |= FA_OPEN_EXISTING;
    }
    if( O_TRUNC == (O_TRUNC & flags) ) {
        mode_flags &= ~FA_OPEN_ALWAYS;
        mode_flags |= FA_CREATE_ALWAYS;
    }

    switch( f_open(file, name, mode_flags) ) {
        case FR_OK:
        {
            DWORD goal;

            r->active[fd] = true;

            goal = 0;
            if( O_APPEND == (O_APPEND & flags) ) {
                goal = file->fsize;
            }

            /* Always seek since FA_OPEN_ALWAYS requires a seek to write. */
            if( FR_OK != f_lseek(file, goal) ) {
                reent->_errno = EBADF;
                return -1;
            }
            return (fd + 3);
        }

        case FR_NO_FILE:
            reent->_errno = ENOENT;
            break;
        case FR_NO_PATH:
            reent->_errno = EFAULT;
            break;
        case FR_INVALID_NAME:
        case FR_INVALID_DRIVE:
        case FR_EXIST:
            reent->_errno = EEXIST;
            break;
        case FR_DENIED:
        case FR_NOT_READY:
        case FR_WRITE_PROTECTED:
            reent->_errno = EROFS;
            break;
        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_ENABLED:
        case FR_NO_FILESYSTEM:
            reent->_errno = ENXIO;
            break;
        default:
            reent->_errno = EACCES;
            break;
    }

    return -1;
}

/**
 *  Used to take a reent struct & a file descriptor & return
 *  the resulting file data.
 *
 *  @param reent the reent structure to search
 *  @param fd the file descriptor to search for
 *
 *  @return the FIL on success, or NULL on error
 */
static FIL* __get_file( struct _reent *reent, int *fd )
{
    struct reent_glue *r = (struct reent_glue*) reent;

    if( REENT_GLUE_MAGIC != r->magic ) {
        reent->_errno = EBADF;
        return NULL;
    }

    *fd -= 3;
    /* Is our fd in range? */
    if( (*fd < 0) || (REENT_GLUE_MAX_FILES <= *fd) ) {
        /* Nope, bail */
        reent->_errno = EBADF;
        return NULL;
    }

    /* Is our fd active? */
    if( false == r->active[*fd] ) {
        /* Nope, bail */
        reent->_errno = EBADF;
        return NULL;
    }

    return (FIL *) r->file[*fd];
}
