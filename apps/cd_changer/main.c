
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include <bsp/boards/boards.h>
#include <bsp/wdt.h>
#include <bsp/cpu.h>
#include <freertos/task.h>
#include <file-stream/file-stream.h>
#include <database/database.h>
#include <memcard/memcard.h>
#include <display/display.h>
#include <dsp/dsp.h>
#include <led/led.h>
#include <media-interface/media-interface.h>
#include <media-flac/media-flac.h>
#include <media-mp3/media-mp3.h>
#include <playback/playback.h>
#include <ibus-phone-protocol/ibus-phone-protocol.h>

#include "radio-interface.h"
#include "device-status.h"
#include "user-interface.h"
#include "ui-default.h"

#define ENABLE_STATUS_TASK      0
#define REPORT_ALL_MALLOC       0

#define ALLOW_USING_SLOW_MEMORY 1

const char firmware_label[] __attribute__ ((section (".firmware_label"))) = "Testing";

void* pvPortMalloc( size_t size )
{
    extern void __sram_heap_start__;
    extern void __sram_heap_end__;
    static int32_t offset = 0;
    static int32_t have = -1;
    void *ret;

    vTaskSuspendAll();

    ret = NULL;

    if( -1 == have ) {
        have = (&__sram_heap_end__ - (&__sram_heap_start__));
    }

    if( size < have ) {
        size_t aligned_size;

        ret = (void *) (&__sram_heap_start__ + offset);

        aligned_size = (0xfffffff8 & (size + 7));

        offset += aligned_size;
        have -= aligned_size;
    } else {
#if (1 == ALLOW_USING_SLOW_MEMORY)
        ret = malloc( size );
#else
        fprintf( stderr, "%s( %lu ) Failure - Needed: %lu Have: %ld - using malloc()\n",
                 __func__, size, size, have );

        fflush( stderr );

        while( 1 ) { ; }
#endif
    }

#if (1 == REPORT_ALL_MALLOC)
    printf( "%s( %lu ) ->: %p (total: %lu left: %ld)\n", __func__, size, ret, offset, have );
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


#if (1 == ENABLE_STATUS_TASK)
static char task_buffer[512];
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

    cpu_disable_orphans();

    printf( "--------------------------------------------------------------------------------\n" );
    wdt_start( WDT__36s );

    vSemaphoreCreateBinary( __debug_mutex );
    vSemaphoreCreateBinary( __debug_block );
    xSemaphoreTake( __debug_block, 0 );

    srand( 12 );

#if (1 == ENABLE_STATUS_TASK)
    xTaskCreate( __idle_task, ( signed portCHAR *) "Status",
                 (configMINIMAL_STACK_SIZE + 200), NULL, tskIDLE_PRIORITY+2, NULL );
#endif

    mi_list = media_new();

    media_register_codec( mi_list, "flac", media_flac_play,
                          media_flac_get_type, media_flac_get_metadata );
    media_register_codec( mi_list, "mp3", media_mp3_play,
                          media_mp3_get_type, media_mp3_get_metadata );

    led_init( (tskIDLE_PRIORITY+1) );
    device_status_init();
    mc_init( pvPortMalloc );
    dsp_init( (tskIDLE_PRIORITY+2) );
    ri_init();
    ui_init();
    ui_t_init();
    playback_init( (tskIDLE_PRIORITY+1) );
    init_database( mi_list );
    fstream_init( (tskIDLE_PRIORITY+2), malloc, free );
    display_init( ibus_phone_display , 5000, 15000, 10000, 1, true);

    /* Start the RTOS - never returns. */
    __enable_os = true;
    vTaskStartScheduler();
    
    return 0;
}
