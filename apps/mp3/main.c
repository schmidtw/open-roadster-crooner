#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <bsp/intc.h>
#include <bsp/led.h>
#include <bsp/memcard.h>
#include <bsp/boards/boards.h>

#include <efsl/efs.h>
#include <efsl/ls.h>

#include <media-mp3/media-mp3.h>

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
void play( EmbeddedFile *file );

int main( void )
{
    EmbeddedFileSystem efs;
    EmbeddedFile file;
    bsp_status_t status;

    enable_global_interrupts();

    led_init();
    led_on( led_all );

    printf( "--------------------------------------------------------------------------------\n" );

    status = mc_init( NULL, NULL, NULL, NULL );

    printf( "mc_init: 0x%08x\n", status );

    while( 1 )
    {
        if( true == mc_present() ) {
            printf( "card is present\n" );
            status = mc_mount();
            printf( "mc_mount: 0x%08x\n", status );
            if( BSP_RETURN_OK == status ) {
                int8_t res;
                res = efs_init( &efs, "/dev/sd" );
                printf( "efs_init: %d\n", res );
                if( 0 == res ) {
                    DirList list;

                    res = ls_openDir( &list, &efs.myFs, "/" );
                    printf( "ls_openDir: %d\n", res );
                    if( 0 == res ) {

                        while( 0 == ls_getNext(&list) ) {
                            list.currentEntry.FileName[LIST_MAXLENFILENAME-1] = '\0';
                            printf( "%c %8lu %s\n", (ATTR_DIRECTORY & list.currentEntry.Attribute) ? 'd' : 'f', list.currentEntry.FileSize, list.currentEntry.FileName );
                        }
                    }

                    res = file_fopen( &file, &efs.myFs, "BOND.MP3", 'r' );
                    printf( "opening BOND.MP3: %d\n", res );

                    if( 0 == res ) {
                        printf( "%p\n", &file );
                        printf( "media_mp3_play: 0x%04x\n", media_mp3_play(&file) );
                        file_fclose( &file );
                    }
                }
                printf( "done!\n" );
                mc_unmount();
            }
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
