
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
#include <media-mp3/media-mp3.h>

#include "radio-interface.h"
#include "playback.h"

#define ENABLE_STATUS_TASK  1
#define REPORT_ALL_MALLOC   0

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
    } else {
        fprintf( stderr, "%s( %lu ) Failure - Needed: %lu Have: %lu\n",
                 __func__, size, size,
                 (&__sram_heap_end__ - (&__sram_heap_start__ + offset)) );

        fflush( stderr );

        while( 1 ) { ; }
    }

#if (1 == REPORT_ALL_MALLOC)
    printf( "%s( %lu ) ->: %p (total: %lu)\n", __func__, size, ret, total );
#endif

    xTaskResumeAll();

    return ret;
}

void vPortFree( void *ptr )
{
    /* We don't free. */
}

static bool __enable_os = false;
static xSemaphoreHandle __debug_mutex;
static xSemaphoreHandle __debug_block;

void _debug_lock( void )
{
    if( true == __enable_os ) {
        xSemaphoreTake( __debug_mutex, portMAX_DELAY );
    }
}

void _debug_unlock( void )
{
    if( true == __enable_os ) {
        xSemaphoreGive( __debug_mutex );
    }
}

void _debug_block( void )
{
    if( true == __enable_os ) {
        xSemaphoreTake( __debug_block, portMAX_DELAY );
    }
}

void _debug_isr_tx_complete( void )
{
    if( true == __enable_os ) {
        portBASE_TYPE ignore;
        xSemaphoreGiveFromISR( __debug_block, &ignore );
    }
}


static char task_buffer[512];
#if (1 == ENABLE_STATUS_TASK)
static void __idle_task( void *params )
{
    while( 1 ) {
        vTaskDelay( 5000 / portTICK_RATE_MS );
        printf( "Still alive\n" );
        vTaskList( (signed portCHAR *) task_buffer );
        printf( "%s\n", task_buffer );
    }
}
#endif

int main( void )
{
    media_interface_t *mi_list = NULL;

    printf( "--------------------------------------------------------------------------------\n" );

    vSemaphoreCreateBinary( __debug_mutex );
    vSemaphoreCreateBinary( __debug_block );
    xSemaphoreTake( __debug_block, 0 );

    srand( 12 );

#if (1 == ENABLE_STATUS_TASK)
    xTaskCreate( __idle_task, ( signed portCHAR *) "Status",
                 (configMINIMAL_STACK_SIZE + 100), NULL, tskIDLE_PRIORITY+2, NULL );
#endif

    mi_list = media_new();

    media_register_codec( mi_list, "flac", media_flac_command,
                          media_flac_play, media_flac_get_type,
                          media_flac_get_metadata );
    media_register_codec( mi_list, "mp3", media_mp3_command,
                          media_mp3_play, media_mp3_get_type,
                          media_mp3_get_metadata );

    mc_init( pvPortMalloc );
    dsp_init( (tskIDLE_PRIORITY+2) );
    ri_init();
    playback_init();
//    display_init( ibus_print , 5000, 15000, 10000, 1, true);
    init_database( mi_list );
    fstream_init( (tskIDLE_PRIORITY+2), malloc, free );

    /* Start the RTOS - never returns. */
    __enable_os = true;
    vTaskStartScheduler();
    
    return 0;
}
