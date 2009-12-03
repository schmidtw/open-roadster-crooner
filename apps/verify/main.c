#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "abdac.h"
#include <bsp/delay.h>
#include <bsp/intc.h>
#include <bsp/gpio.h>
#include <led/led.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <linked-list/linked-list.h>

/* Integers */
#define SDRAM_SIZE  8388608

static xSemaphoreHandle __card_state_change;

void echo_task( void *params );
void test_sdram( void );
void abdac_task( void *params );
void simple_test_task( void *params );
void abdac_done( abdac_node_t *node, const bool last );
void card_task( void *params );
static bool is_card_present( void );
__attribute__((__interrupt__))
static void __card_change( void );
static bool is_card_wp( void );

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
    led_init( tskIDLE_PRIORITY + 1 );

    vSemaphoreCreateBinary( __card_state_change );
    xSemaphoreTake( __card_state_change, 0 );

    xTaskCreate( simple_test_task, (signed portCHAR *) "TEST", 2000, NULL, (tskIDLE_PRIORITY+1), NULL );
    xTaskCreate(        echo_task, (signed portCHAR *) "ECHO", 2000, NULL, (tskIDLE_PRIORITY+1), NULL );
    xTaskCreate(       abdac_task, (signed portCHAR *) "DAC ", 2000, NULL, (tskIDLE_PRIORITY+1), NULL );
    xTaskCreate(        card_task, (signed portCHAR *) "CARD", 2000, NULL, (tskIDLE_PRIORITY+1), NULL );

    printf( "--------------------------------------------------------------------------------\n" );

    /* Start the RTOS - never returns. */
    vTaskStartScheduler();
    
    return 0;
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

void led_red()
{
    led_state_t states = { .red = 255, .green = 0, .blue = 0, .duration = 0 };
    led_set_state( &states, 1, false, NULL );
}

void simple_test_task( void *params )
{
    led_state_t states[] = { { .red = 255, .green =   0, .blue =   0, .duration = 2000 },
                             { .red =   0, .green = 255, .blue =   0, .duration = 2000 },
                             { .red =   0, .green =   0, .blue = 255, .duration = 2000 },
                             { .red = 255, .green =   0, .blue = 255, .duration = 2000 },
                             { .red = 255, .green = 255, .blue =   0, .duration = 2000 },
                             { .red = 255, .green = 255, .blue = 255, .duration = 2000 },
                             { .red =   0, .green = 255, .blue = 255, .duration = 2000 },
                             { .red =   0, .green =   0, .blue =   0, .duration = 2000 } };

    led_set_state( states, sizeof(states)/sizeof(led_state_t), true, NULL );

    test_sdram();
}

void test_sdram( void )
{
    uint32_t *sdram;
    int i;

    sdram = (uint32_t*) (0xD0000000);

    printf( "Starting SDRAM Test\n" );

    printf( "Writing 0xffffffff\n" );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        sdram[i] = 0xffffffff;
    }

    printf( "Verifying 0xffffffff\n" );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        if( 0xffffffff != sdram[i] ) {
            printf( "Failed SDRAM Test\n" );
            led_red();
            while( 1 ) {;}
        }
    }

    printf( "Writing Sequential\n" );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        sdram[i] = i;
    }

    printf( "Verifying Sequential\n" );
    for( i = 0; i < SDRAM_SIZE; i++ ) {
        if( i != sdram[i] ) {
            printf( "Failed SDRAM Test\n" );
            led_red();
            while( 1 ) {;}
        }
    }

    printf( "SDRAM Test Succeeded\n" );

    while( 1 ) { ; }
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
        isrs_enabled = interrupts_save_and_disable();
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

void card_task( void *params )
{
    /* Create an interrupt on the card ejection. */
    gpio_set_options( MC_CD_PIN,
                      GPIO_DIRECTION__INPUT,
                      GPIO_PULL_UP__DISABLE,
                      GPIO_GLITCH_FILTER__ENABLE,
                      GPIO_INTERRUPT__CHANGE,
                      0 );
    gpio_set_options( MC_WP_PIN,
                      GPIO_DIRECTION__INPUT,
                      GPIO_PULL_UP__DISABLE,
                      GPIO_GLITCH_FILTER__ENABLE,
                      GPIO_INTERRUPT__NONE,
                      0 );

    intc_register_isr( &__card_change, MC_CD_ISR, ISR_LEVEL__2 );

    /* In case we booted with a card in the slot. */
    if( true == is_card_present() ) {
        xSemaphoreGive( __card_state_change );
    }

    while( 1 ) {
        xSemaphoreTake( __card_state_change, portMAX_DELAY );

        if( true == is_card_present() ) {
            if( true == is_card_wp() ) {
                printf( "Card is present & locked.\n" );
            } else {
                printf( "Card is present & unlocked.\n" );
            }
        } else {
            printf( "Card is not present.\n" );
        }
    }
}

static bool is_card_present( void )
{
#if (0 != MC_CD_ACTIVE_LOW)
    return (0 == gpio_read_pin(MC_CD_PIN));
#else
    return (0 != gpio_read_pin(MC_CD_PIN));
#endif
}

static bool is_card_wp( void )
{
#if (0 != MC_WP_ACTIVE_LOW)
    return (0 == gpio_read_pin(MC_WP_PIN));
#else
    return (0 != gpio_read_pin(MC_WP_PIN));
#endif
}


__attribute__((__interrupt__))
static void __card_change( void )
{
    portBASE_TYPE ignore;
    
    xSemaphoreGiveFromISR( __card_state_change, &ignore );

    gpio_clr_interrupt_flag( MC_CD_PIN );
}
