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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/reent.h>

#include <newlib/reent-file-glue.h>

#include "fatfs/ff.h"
#include "dirent.h"

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
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static int _closedir_r( struct _reent *reent, DIR *dirp );
static DIR* _opendir_r( struct _reent *reent, const char *dirname );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int closedir( DIR *dirp )
{
    return _closedir_r( _REENT, dirp );
}

DIR* opendir( const char *dirname )
{
    return _opendir_r( _REENT, dirname );
}

struct dirent* readdir( DIR *dirp, struct dirent *_user_provided )
{
    if( NULL == _user_provided ) {
        errno = EINVAL;
        return NULL;
    }

    if( NULL != dirp ) {
        FILINFO file_info;

        if( 0 != dirp->end ) {
            return NULL;
        }

        switch( f_readdir(dirp, &file_info) ) {
            case FR_OK:
                if( '\0' == file_info.fname[0] ) {
                    /* Last entry. */
                    dirp->end = 1;
                    return NULL;
                }

                _user_provided->d_ino = 0;
                _user_provided->d_size = file_info.fsize;
                _user_provided->d_attr = 0;
                if( AM_RDO == (AM_RDO & file_info.fattrib) ) {
                    _user_provided->d_attr |= DT_READ_ONLY;
                }
                if( AM_DIR == (AM_DIR & file_info.fattrib) ) {
                    _user_provided->d_attr |= DT_DIR;
                }
                memcpy( _user_provided->d_name, file_info.fname, (DT_NAME_MAX+1) );
                return _user_provided;

            case FR_NOT_READY:
            case FR_DISK_ERR:
            case FR_INT_ERR:
            case FR_INVALID_OBJECT:
            default:
                break;
        }
    }

    errno = EBADF;
    return NULL;
}

void rewinddir( DIR *dirp )
{
    if( NULL == dirp ) {
        return;
    }

    f_readdir( dirp, NULL );
    dirp->end = 0;
}

void seekdir( DIR *dirp, long loc )
{
    if( NULL == dirp ) {
        return;
    }

    while( (0 == dirp->end) && (dirp->index < loc) ) {
        struct dirent ignore;
        readdir( dirp, &ignore );
    }

}

long telldir( DIR *dirp )
{
    if( NULL == dirp ) {
        errno = EBADF;
        return -1;
    }

    return (long) dirp->index;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static int _closedir_r( struct _reent *reent, DIR *dirp )
{
    struct reent_glue *r = (struct reent_glue*) reent;
    int fd;

    if( REENT_GLUE_MAGIC == r->magic ) {
        for( fd = 0; fd < REENT_GLUE_MAX_FILES; fd++ ) {
            if( true == r->active[fd] ) {
                if( dirp == (DIR*) r->file[fd] ) {
                    /* Mark this slot as not in use & we're done. */
                    r->active[fd] = false;
                    return 0;
                }
            }
        }
    }

    reent->_errno = EBADF;
    return -1;
}

static DIR* _opendir_r( struct _reent *reent, const char *dirname )
{
    struct reent_glue *r = (struct reent_glue*) reent;
    int fd;
    DIR *dir;

    if( REENT_GLUE_MAGIC != r->magic ) {
        reent->_errno = ENFILE;
        return NULL;
    }

    for( fd = 0; fd < REENT_GLUE_MAX_FILES; fd++ ) {
        if( false == r->active[fd] ) {
            /* We found an empty fd. */
            break;
        }
    }

    if( REENT_GLUE_MAX_FILES == fd ) {
        reent->_errno = ENFILE;
        return NULL;
    }

    if( NULL == r->file[fd] ) {
        r->file[fd] = (unsigned char *) malloc( sizeof(FF_SLOT) );
        if( NULL == r->file[fd] ) {
            reent->_errno = ENOMEM;
            return NULL;
        }
    }

    dir = (DIR *) r->file[fd];

    switch( f_opendir(dir, dirname) ) {
        case FR_OK:
            r->active[fd] = true;
            dir->end = 0;
            return dir;

        case FR_NO_PATH:
        case FR_INVALID_NAME:
            reent->_errno = ENOENT;
            break;
        case FR_NOT_READY:
        case FR_DISK_ERR:
        case FR_INT_ERR:
        case FR_NOT_ENABLED:
        case FR_NO_FILESYSTEM:
        default:
            reent->_errno = EACCES;
            break;
    }

    return NULL;
}
