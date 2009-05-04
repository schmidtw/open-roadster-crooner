#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <bsp/abdac.h>
#include <bsp/delay.h>
#include <bsp/intc.h>
#include <bsp/led.h>
#include <freertos/task.h>
#include <linked-list/linked-list.h>

/* Integers */
#define SDRAM_SIZE  8388608

#define COLOR_WHITE     0
#define COLOR_NONE      1
#define COLOR_RED       2
#define COLOR_GREEN     3
#define COLOR_BLUE      4
#define COLOR_YELLOW    5
#define COLOR_MAGENTA   6
#define COLOR_CYAN      7

void led_task( void *params );
void echo_task( void *params );
void test_sdram( void );
void test_led( const int color );
void abdac_task( void *params );
void abdac_done( abdac_node_t *node, const bool last );

#include "audio.included"

void* pvPortMalloc( size_t size )
{
    void *ret;

    vTaskSuspendAll();
    ret = malloc( size );
    printf( "%p = malloc( %ld )\n", ret, size );
    xTaskResumeAll();

    return ret;
}

void vPortFree( void *ptr )
{
    vTaskSuspendAll();
    printf( "free( %p )\n", ptr );
    free( ptr );
    xTaskResumeAll();
}

int main( void )
{
    int i;

    led_init();

    for( i = 0; i < 8; i++ ) {
        test_led( i );
        delay_time( 20000000 );
    }
    test_led( 1 );

    test_sdram();

    xTaskCreate(   led_task, (signed portCHAR *) "LED ",  400, NULL, (tskIDLE_PRIORITY+1), NULL );
    xTaskCreate(  echo_task, (signed portCHAR *) "ECHO", 1000, NULL, (tskIDLE_PRIORITY+1), NULL );
    xTaskCreate( abdac_task, (signed portCHAR *) "DAC ",  400, NULL, (tskIDLE_PRIORITY+1), NULL );

    printf( "--------------------------------------------------------------------------------\n" );

    /* Start the RTOS - never returns. */
    vTaskStartScheduler();
    
    return 0;
}

void led_task( void *params )
{
    while( 1 ) {
        int i;

        for( i = 0; i < 8; i++ ) {
            test_led( i );
            vTaskDelay( 1000 );
        }
    }
}

void echo_task( void *params )
{
    while( 1 ) {
#if 0
        int c;
        printf( "1 char, then enter: " );
        printf( "scanf: %d\n", scanf("%i", &c) );
        printf( "Got: %d\n", c );
#else
        printf( "Serial Input Not Currently Supported.\n" );
#endif
        vTaskDelay( 10000 );
    }
}

void test_sdram( void )
{
    uint32_t *sdram;
    int i;

    sdram = (uint32_t*) (0xD0000000);

    test_led( COLOR_BLUE );

    for( i = 0; i < SDRAM_SIZE; i++ ) {
        sdram[i] = 0xffffffff;
    }

    test_led( COLOR_YELLOW );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        if( 0xffffffff != sdram[i] ) {
            test_led( COLOR_RED );
            while( 1 ) {;}
        }
    }

    test_led( COLOR_CYAN );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        sdram[i] = i;
    }

    test_led( COLOR_WHITE );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        if( i != sdram[i] ) {
            test_led( COLOR_RED );
            while( 1 ) {;}
        }
    }

    test_led( COLOR_GREEN );
}

void test_led( const int color )
{
    switch( color ) {
        case COLOR_WHITE:
            led_on( led_red );
            led_on( led_green );
            led_on( led_blue );
            break;

        case COLOR_NONE:
            led_off( led_red );
            led_off( led_green );
            led_off( led_blue );
            break;

        case COLOR_RED:
            led_on( led_red );
            led_off( led_green );
            led_off( led_blue );
            break;

        case COLOR_GREEN:
            led_off( led_red );
            led_on( led_green );
            led_off( led_blue );
            break;

        case COLOR_BLUE:
            led_off( led_red );
            led_off( led_green );
            led_on( led_blue );
            break;

        case COLOR_YELLOW:
            led_on( led_red );
            led_on( led_green );
            led_off( led_blue );
            break;

        case COLOR_MAGENTA:
            led_on( led_red );
            led_off( led_green );
            led_on( led_blue );
            break;

        case COLOR_CYAN:
            led_off( led_red );
            led_on( led_green );
            led_on( led_blue );
            break;

        default:
            break;
    }
}

volatile ll_list_t __idle;
void abdac_task( void *params )
{
    int count;
    ll_node_t *node;
    abdac_node_t nodes[2];

    ll_init_list( &__idle );

    ll_init_node( &nodes[0].node, &nodes[0] );
    nodes[0].buffer = (uint8_t*) audio;
    nodes[0].size = sizeof(audio);

    ll_init_node( &nodes[1].node, &nodes[1] );
    nodes[1].buffer = (uint8_t*) audio;
    nodes[1].size = sizeof(audio);

    abdac_init( false, abdac_done );
    abdac_set_sample_rate( ABDAC_SAMPLE_RATE__44100 );
    abdac_queue_silence();
    abdac_queue_data( &nodes[0], false );
    abdac_queue_data( &nodes[1], false );
    abdac_start();

    count = 0;
    while( 1 ) {
        bool isrs_enabled;
        interrupts_save_and_disable( isrs_enabled );
        node = ll_remove_head( &__idle );
        interrupts_restore( isrs_enabled );

        if( NULL == node ) {
            vTaskDelay( 100 );
        } else {
            abdac_queue_data( (abdac_node_t*) node->data, false );
        }
        if( 0 == count ) {
            gpio_set_pin( AUDIO_DAC_MUTE_PIN );
        } else if( 10 == count ) {
            gpio_clr_pin( AUDIO_DAC_MUTE_PIN );
        } else if( 20 == count ) {
            count = -1;
        }
        count++;
    }
}

void abdac_done( abdac_node_t *node, const bool last )
{
    ll_append( &__idle, &node->node );
}
