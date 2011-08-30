
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#include <bsp/boards/boards.h>
#include <bsp/wdt.h>
#include <bsp/cpu.h>
#include <freertos/os.h>
#include <file-stream/file-stream.h>
#include <database/database.h>
#include <memcard/memcard.h>
#ifdef SUPPORT_TEXT
#include <display/display.h>
#endif
#include <dsp/dsp.h>
#include <led/led.h>
#include <media-interface/media-interface.h>
#include <media-flac/media-flac.h>
#include <media-mp3/media-mp3.h>
#include <playback/playback.h>
#include <ibus-phone-protocol/ibus-phone-protocol.h>
#include <system-time/system-time.h>

#include "reboot-sys.h"
#include "radio-interface.h"
#include "device-status.h"
#include "user-interface.h"
#include "ui-t.h"

#define ENABLE_STATUS_TASK      0
#define REPORT_ALL_MALLOC       0

#define ALLOW_USING_SLOW_MEMORY 1

const char firmware_label[] __attribute__ ((section (".firmware_label"))) = "Crooner-1.0.15";

static int32_t __sram_offset = 0;
static int32_t __sram_have = -1;
static int32_t __sdram_use = 0;
void* pvPortMalloc( size_t size )
{
    extern void __sram_heap_start__;
    extern void __sram_heap_end__;
    void *ret;

    os_task_suspend_all();

    ret = NULL;

    if( -1 == __sram_have ) {
        __sram_have = (&__sram_heap_end__ - (&__sram_heap_start__));
    }

    if( size < __sram_have ) {
        size_t aligned_size;

        ret = (void *) (&__sram_heap_start__ + __sram_offset);

        aligned_size = (0xfffffff8 & (size + 7));

        __sram_offset += aligned_size;
        __sram_have -= aligned_size;
    } else {
#if (1 == ALLOW_USING_SLOW_MEMORY)
        ret = malloc( size );
        __sdram_use += size;
#else
        fprintf( stderr, "%s( %lu ) Failure - Needed: %lu Have: %ld - using malloc()\n",
                 __func__, size, size, __sram_have );

        fflush( stderr );

        while( 1 ) { ; }
#endif
    }

#if (1 == REPORT_ALL_MALLOC)
    printf( "%s( %lu ) ->: %p (total: %lu left: %ld)\n", __func__, size, ret, __sram_offset, __sram_have );
#endif

    os_task_resume_all();

    return ret;
}

void vPortFree( void *ptr )
{
    /* We don't free. */
}

static bool __enable_os = false;
static semaphore_handle_t __debug_mutex;
static semaphore_handle_t __debug_block;

void _debug_lock( void )
{
    if( true == __enable_os ) {
        os_semaphore_take( __debug_mutex, WAIT_FOREVER );
    }
}

void _debug_unlock( void )
{
    if( true == __enable_os ) {
        os_semaphore_give( __debug_mutex );
    }
}

void _debug_block( void )
{
    if( true == __enable_os ) {
        os_semaphore_take( __debug_block, WAIT_FOREVER );
    }
}

void _debug_isr_tx_complete( void )
{
    if( true == __enable_os ) {
        os_semaphore_give_ISR( __debug_block, NULL );
    }
}

#if (1 == ENABLE_STATUS_TASK)
static char task_buffer[1024];
static void __idle_task( void *params )
{
    while( 1 ) {
        os_task_delay_ms( 5000 );
        printf( "Still alive, SRAM Left: %ld, OS/Stack SDRAM Usage: %ld\n", __sram_have, __sdram_use );
        os_task_get_run_time_stats( task_buffer );
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

     __debug_mutex = os_semaphore_create_binary();
     __debug_block = os_semaphore_create_binary();
    os_semaphore_take( __debug_block, NO_WAIT );

    srand( 12 );

#if (1 == ENABLE_STATUS_TASK)
    os_task_create( __idle_task, "Status", 550, NULL, 2, NULL );
#endif

    mi_list = media_new();

    media_register_codec( mi_list, "flac", media_flac_play,
                          media_flac_get_type, media_flac_get_metadata );
    media_register_codec( mi_list, "mp3", media_mp3_play,
                          media_mp3_get_type, media_mp3_get_metadata );

    led_init( 1 );
    device_status_init();
    mc_init( pvPortMalloc );
    dsp_init( 2 );
    ri_init();
    ui_init();
    ui_t_init();
    //uid_init();
    playback_init( 1 );
    init_database( mi_list );
    fstream_init( 2, malloc, free );
    system_time_init(1);
#ifdef SUPPORT_TEXT
    display_init( ibus_phone_display, 1000, 4000, 2000, 5000, 3, true);
#endif

    reboot_system_init();

    /* Start the RTOS - never returns. */
    __enable_os = true;
    os_task_start_scheduler();
    
    return 0;
}
