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
#include <media-flac/media-flac.h>
#include <efsl/efs.h>
#include <efsl/ls.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <util/xxd.h>

#include "blu.h"
#include "wav.h"
//#include "mp3.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PB_TASK_STACK_SIZE     (9120)
#define PB_TASK_PRIORITY       (tskIDLE_PRIORITY+1)
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    bool dac_setup;
    EmbeddedFile file;
} pb_cb_info_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params );
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

                play();
            }
        } else {
            if( true == card_present ) {
                disc_status_message_t *msg;

                /* State change - card removed. */
                printf( "card removed.\n" );
                card_present = false;

                msg = disc_status_message_alloc();
                msg->disc_status = BLU_DISC_STATUS__NO_MAGAZINE;
                blu_message_post( msg );

            }
        }
        //printf( "here\n" );
    }
}

void play( void )
{
    bsp_status_t status;
    status = mc_mount();
    printf( "mount status: 0x%08x\n", status );
    if( BSP_RETURN_OK == status ) {
        EmbeddedFileSystem efs;
        int8_t res;
        res = efs_init( &efs, "/dev/sd0" );
        printf( "res: %d\n", res );
        if( 0 == res ) {
            DirList list;
            pb_cb_info_t info;

            led_on( led_blue );

            ls_openDir( &list, &efs.myFs, "/2" );
            while( 0 == ls_getNext(&list) ) {
                list.currentEntry.FileName[LIST_MAXLENFILENAME-1] = '\0';
                printf( "%c %8lu %s\n", (ATTR_DIRECTORY & list.currentEntry.Attribute) ? 'd' : 'f', list.currentEntry.FileSize, list.currentEntry.FileName );
                fflush( stdout );
            }
            printf( "done\n" );
            fflush( stdout );

            info.dac_setup = false;
#if 1
            while( 1 ) {
                int i;
                for( i = 0; i < 13; i++ ) {
                    char *name;

                    switch( i ) {
                        //case 0: name = "/2/01-RAD~1.FLA"; break;
                        case 0: name = "/1.FLA"; break;
                        case 1: name = "/2.FLA"; break;
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
#else
            //res = file_fopen( &info.file, &efs.myFs, "END.WAV", 'r' );
            //res = file_fopen( &info.file, &efs.myFs, "SWING2.WAV", 'r' );
            //res = file_fopen( &info.file, &efs.myFs, "SILENCE.WAV", 'r' );
            //if( 0 == res ) {
                while( 1 ) {
                    if( 0 == file_fopen(&info.file, &efs.myFs, "1.WAV", 'r') ) {
                        wav_play( &info.file );
                        file_fclose( &info.file );
                        printf( "wav_file done.\n" );
                    }
                    if( 0 == file_fopen(&info.file, &efs.myFs, "PUPPY.WAV", 'r') ) {
                        wav_play( &info.file );
                        file_fclose( &info.file );
                        printf( "wav_file done.\n" );
                    }
                    if( 0 == file_fopen(&info.file, &efs.myFs, "END.WAV", 'r') ) {
                        wav_play( &info.file );
                        file_fclose( &info.file );
                        printf( "wav_file done.\n" );
                    }
                    //file_setpos( &info.file, 0 );
                    //printf( "file closed" );
                }
            //}
#endif
        }

        printf( "mc_unmount: 0x%08x\n", mc_unmount() );
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
