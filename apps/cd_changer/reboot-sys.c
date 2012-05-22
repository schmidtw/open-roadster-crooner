/*
 * Copyright (c) 2011  Weston Schmidt
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <freertos/os.h>
#include <bsp/reboot.h>
#include <memcard/dirent.h>
#include <memcard/memcard.h>

#include "reboot-sys.h"

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
static void __reboot_task( void *params );
static FILE *__open_next_log_file( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void reboot_system_init( void )
{
    os_task_create( __reboot_task, "Reboot", 550, NULL, 2, NULL );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __reboot_task( void *params )
{
    reboot_trace_t *last;
    bool last_reset_reported;

    last = reboot_get_last();
    last_reset_reported = false;


    if( NULL == last ) {
        printf( "No reboots to report...\n" );
        goto done;
    }
    fprintf( stdout, "Last Reboot Information:\n" );
    fprintf( stdout, "========================================\n" );
    reboot_output( stdout, last );

    while( false == last_reset_reported ) {
        FILE *fp;

        while( MC_CARD__MOUNTED != mc_get_status() ) {
            os_task_delay_ms( 1000 );
        }

        fp = __open_next_log_file();
        if( NULL != fp ) {
            reboot_output( fp, last );
            fclose( fp );
            last_reset_reported = true;
        } else {
            os_task_delay_ms( 1000 );
        }
    }

done:
    while( 1 ) {
        /* Wake up once a day(ish). */
        os_task_delay_ms( 1000 * 60 * 60 * 24 );
    }
}

static FILE *__open_next_log_file( void )
{
    DIR *dir;
    struct dirent file_info;
    struct dirent *ent;
    int old_errno;
    uint32_t count;
    char filename[14];

    count = 0;
    ent = NULL;
    memset( filename, '\0', sizeof(filename) );

    dir = opendir( "/" );
    if( NULL == dir ) {
        return NULL;
    }

    /* Find the next available reboot filename "reboot.xxx". */
    old_errno = errno;
    do {
        ent = readdir( dir, &file_info );
        if( NULL == ent ) {
            if( old_errno != errno ) {
                closedir( dir );
                return NULL;
            }
        } else {
            /* Ignore directories */
            if( !(ent->d_attr & DT_DIR) ) {
                if( 0 == strncasecmp("reboot", ent->d_name, 6) ) {
                    uint32_t num;

                    if( 1 == sscanf(&ent->d_name[6], ".%lu", &num) ) {
                        if( count <= num ) {
                            count = num + 1;
                        }
                    }
                }
            }
        }
    } while( NULL != ent );

    closedir( dir );

    snprintf( filename, sizeof(filename) - 1, "reboot.%lu", count );
    printf( "Reboot logger logging to: '%s'\n", filename );

    return fopen( filename, "w" );

}
