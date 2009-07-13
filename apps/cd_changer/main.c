
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include <bsp/boards/boards.h>
#include <freertos/task.h>
#include <file-stream/file-stream.h>
#include <display/display.h>
#include <database/database.h>
#include <memcard/memcard.h>
#include <display/display.h>
#include <dsp/dsp.h>
#include <media-interface/media-interface.h>
#include <media-flac/media-flac.h>

#include "radio-interface.h"
#include "playback.h"

void* pvPortMalloc( size_t size )
{
    extern void __sram_heap_start__;
    extern void __sram_heap_end__;
    static size_t offset = 0;
    static size_t total = 0;
    void *ret;

    vTaskSuspendAll();

    ret = NULL;

    if( (&__sram_heap_start__ + offset + size) < &__sram_heap_end__ ) {
        ret = (void *) (&__sram_heap_start__ + offset);
        offset += (0xfffffff8 & (size + 7));

        total += size;
    }

    if( NULL == ret ) {
        while( 1 ) { ; }
    }
    //printf( "pvPortMalloc( %lu ) ->: %p (total: %lu)\n", size, ret, total );

    xTaskResumeAll();

    return ret;
}

void vPortFree( void *ptr )
{
    /* We don't free. */
}

int main( void )
{
    media_interface_t *mi_list = NULL;
    
    printf( "--------------------------------------------------------------------------------\n" );

    srand( 12 );

    mi_list = media_new();

    media_register_codec( mi_list, "flac", media_flac_command,
                          media_flac_play, media_flac_get_type,
                          media_flac_get_metadata );

    mc_init( pvPortMalloc );
    dsp_init( (tskIDLE_PRIORITY+2) );
    ri_init();
    playback_init();
//    display_init( ibus_print , 5000, 15000, 10000, 1, true);
    init_database( mi_list );
    fstream_init( (tskIDLE_PRIORITY+2), malloc, free );

    /* Start the RTOS - never returns. */
    vTaskStartScheduler();
    
    return 0;
}
