#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr32/io.h>
#include <bsp/boards/boards.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/usart.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <file-stream/file-stream.h>
#include <reent.h>
#include <memcard/memcard.h>

#include "ibus.h"
#include "blu.h"
#include "playback.h"

#include "display/display.h"

void* pvPortMalloc( size_t size )
{
    extern void __sram_heap_start__;
    extern void __sram_heap_end__;
    static size_t offset = 0;
    void *ret;

    vTaskSuspendAll();

    ret = NULL;

    if( (&__sram_heap_start__ + offset + size) < &__sram_heap_end__ ) {
        ret = (void *) (&__sram_heap_start__ + offset);
        offset += (0xfffffff8 & (size + 7));
    }

    //printf( "pvPortMalloc( %d ) ->: %p\n", size, ret );

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
    ibus_init();
    blu_init();
    playback_init();
    //display_init( ibus_print , 5000, 15000, 10000, 1, true);
    fstream_init( (tskIDLE_PRIORITY+2), malloc, free );
    /* Start the RTOS - never returns. */
    vTaskStartScheduler();
    
    return 0;
}
