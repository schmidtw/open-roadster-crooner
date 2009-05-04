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

#include <media-flac/media-flac.h>

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

    if( true == mc_present() ) {
        printf( "card is present\n" );
        status = mc_mount();
        printf( "mc_mount: 0x%08x\n", status );
        if( BSP_RETURN_OK == status ) {
            int8_t res;
            res = efs_init( &efs, "/dev/sd" );
            printf( "res: %d\n", res );
            if( 0 == res ) {
                DirList list;
                ls_openDir( &list, &efs.myFs, "/" );

                while( 0 == ls_getNext(&list) ) {
                    list.currentEntry.FileName[LIST_MAXLENFILENAME-1] = '\0';
                    printf( "%c %8lu %s\n", (ATTR_DIRECTORY & list.currentEntry.Attribute) ? 'd' : 'f', list.currentEntry.FileSize, list.currentEntry.FileName );
                }
                res = file_fopen( &file, &efs.myFs, "PUPPY~1.FLA", 'r' );
                printf( "opening PUPPY~1.FLA: %d\n", res );

                printf( "%p\n", &file );
                printf( "media_flac_play: 0x%04x\n", media_flac_play(&file) );
            }
        }
    }

    printf( "done!\n" );
    while( 1 ) {;}

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
