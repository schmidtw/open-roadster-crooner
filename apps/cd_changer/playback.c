/*
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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <bsp/boards/boards.h>
#include <bsp/boards/boards.h>
#include <bsp/led.h>
#include <bsp/gpio.h>
#include <bsp/memcard.h>
#include <bsp/abdac.h>
#include <bsp/pm.h>
#include <fatfs/ff.h>
#include <media-flac/media-flac.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <util/xxd.h>

#include "blu.h"
//#include "wav.h"
//#include "mp3.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PB_TASK_STACK_SIZE     (9120)
#define PB_TASK_PRIORITY       (tskIDLE_PRIORITY+1)
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params );
void list_dirs( void );
void play( void );
static void mc_suspend( void );
static void mc_resume( void );
static void codec_suspend( void );
static void codec_resume( void );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile xSemaphoreHandle __mc_semaphore;
static volatile xSemaphoreHandle __codec_semaphore;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void playback_init( void )
{
    bsp_status_t status;

    printf( "playback_init()\n" );

    vSemaphoreCreateBinary( __mc_semaphore );
    xSemaphoreTake( __mc_semaphore, 0 );
    vSemaphoreCreateBinary( __codec_semaphore );

    status = mc_init( NULL, NULL, vTaskDelay, mc_suspend, mc_resume, NULL );

    printf( "mc_init: 0x%08x\n", status );

    xTaskCreate( __pb_task, ( signed portCHAR *) "PLYB",
                 PB_TASK_STACK_SIZE, NULL, PB_TASK_PRIORITY, NULL );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params )
{
    FATFS fs;
    bool card_present;

    card_present = false;

    while( 1 ) {
        vTaskDelay( 1000 );
        if( true == mc_present() ) {
            if( false == card_present ) {
                disc_status_message_t *msg;

                /* State change - card inserted. */
                printf( "card inserted.\n" );
                card_present = true;

                msg = disc_status_message_alloc();
                msg->disc_status = BLU_DISC_STATUS__MAGAZINE_PRESENT | BLU_DISC_STATUS__DISC_2;
                blu_message_post( msg );

                printf( "f_mount: %d\n", f_mount(0, &fs) );

                list_dirs();
                play();
            }
        } else {
            if( true == card_present ) {
                disc_status_message_t *msg;

                /* State change - card removed. */
                printf( "card removed.\n" );
                printf( "f_mount: %d\n", f_mount(0, NULL) );
                card_present = false;

                msg = disc_status_message_alloc();
                msg->disc_status = BLU_DISC_STATUS__NO_MAGAZINE;
                blu_message_post( msg );

            }
        }
        //printf( "here\n" );
    }
}

void list_dirs( void )
{
    DIR dir;
    FILINFO info;
    bool done;
    char filename[20];
    BYTE rv;
#if _USE_LFN
    char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];

    info.lfname = lfn;
    info.lfsize = sizeof(lfn);
#endif

            led_on( led_blue );

    rv = f_opendir( &dir, "/" );
    printf( "f_opendir: %d\n", rv );
    if( 0 != rv ) {
        return;
            }

    done = false;
    while( false == done ) {
        f_readdir( &dir, &info );
        if( '\0' == info.fname[0] ) {
            //done = true;
        } else {
            char *name = info.fname;
#if _USE_LFN
            name = info.lfname;
#endif
            if( AM_DIR == (AM_DIR & info.fattrib) ) {
                printf( "/%s\n", name );
            } else {
                printf( "%u f '%s' '%s'\n", info.fsize, info.fname, name );
                filename[0] = '/';
                //filename[1] = '3';
                //filename[2] = '/';
                strcpy( &filename[1], name );
                media_flac_play( "/3/01-RAD~1.FLA", codec_suspend, codec_resume );
                while( 1 ) {;}
            }
        }
    }
}

void play( void )
{
            while( 1 ) {
                int i;
        for( i = 1; i < 13; i++ ) {
                    char *name;

                    switch( i ) {
                case 0: name = "/2/01-RAD~1.FLA"; break;
                case 1: name = "/2/02-PIL~1.FLA"; break;
                //case 0: name = "/1.FLA"; break;
                //case 1: name = "/2.FLA"; break;
                        case 2: name = "/4.FLA"; break;
                        case 3: name = "/5.FLA"; break;
                        case 4: name = "/5.FLA"; break;
                        case 5: name = "/6.FLA"; break;
                        case 6: name = "/7.FLA"; break;
                        case 7: name = "/8.FLA"; break;
                        case 8: name = "/9.FLA"; break;
                        case 9: name = "/10.FLA"; break;
                        case 10: name = "/11.FLA"; break;
                        case 11: name = "/12.FLA"; break;
                        case 12: name = "/PUPPY~1.FLA"; break;
                    }

            printf( "Playing file: '%s'\n", name );
            media_flac_play( name, codec_suspend, codec_resume );
        }
/*
                    if( 0 == file_fopen(&info.file, &efs.myFs, name, 'r') ) {
                        printf( "Wts\n" );
                        media_flac_play( &info.file, codec_suspend, codec_resume );
                        file_fclose( &info.file );
                        printf( "done\n" );
                    } else {
                        printf( "failed to open: '%s'\n", name );
                    }
                    if( false == mc_present() ) {
                        printf( "mc_unmount: 0x%08x\n", mc_unmount() );
                        return;
                    }
                }
            }
*/
    }
//#endif
}

/**
 *  Block the memory card reading task using a semaphore instead
 *  of spin-locking.
 */
static void mc_suspend( void )
{
    xSemaphoreTake( __mc_semaphore, portMAX_DELAY );
}

/**
 *  Resume the memory card reading task from inside an ISR.
 */
static void mc_resume( void )
{
    portBASE_TYPE xTaskWoken;
    xSemaphoreGiveFromISR( __mc_semaphore, &xTaskWoken );
}

/**
 *  Block the codec decoding task using a semaphore instead
 *  of spin-locking.
 */
static void codec_suspend( void )
{
    xSemaphoreTake( __codec_semaphore, portMAX_DELAY );
}

/**
 *  Resume the codec decoding task from inside an ISR.
 */
static void codec_resume( void )
{
    portBASE_TYPE xTaskWoken;
    xSemaphoreGiveFromISR( __codec_semaphore, &xTaskWoken );
}
