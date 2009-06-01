
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
    printf( "--------------------------------------------------------------------------------\n" );

    mc_init( pvPortMalloc );
    ri_init();
    playback_init();
//    display_init( ibus_print , 5000, 15000, 10000, 1, true);
    init_database();
    fstream_init( (tskIDLE_PRIORITY+2), malloc, free );

    /* Start the RTOS - never returns. */
    vTaskStartScheduler();
    
    return 0;
}
