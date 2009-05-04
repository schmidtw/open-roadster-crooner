#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "display.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define NUM_CHARS_ON_LCD 13

static char test_string[500];
static char comparison_string[NUM_CHARS_ON_LCD+1];
static char local_string[NUM_CHARS_ON_LCD+1];

static bool handle_msg_action_rv = false;

size_t wrapper_printf(char *string)
{
    int32_t num;
    
    num = sprintf(local_string, "%.*s", NUM_CHARS_ON_LCD, string);
    printf("%.*s  ", NUM_CHARS_ON_LCD, string );
    if( num > -1 ) {
        return (size_t)num;
    }
    return 0;
}

void handle_display_update( portTickType * ticks_to_wait_before_looping,
                            char * text,
                            struct display_globals * ref )
{
    return;
}

bool handle_msg_action( struct display_message * msg,
                        portTickType * ticks_to_wait_for_message,
                        portTickType * ticks_to_wait_before_looping,
                        char * text,
                        struct display_globals * ref )
{
    return handle_msg_action_rv;
}

size_t broken_printf(char *string)
{
    return 0;
}

void init_fake_stuff( void )
{
    fake_queue_create = 0;
    fake_queue_receive = 0;
    fake_semaphore_create = 0;
    fake_x_task_create = NULL;
}

void test_init_destroy( void )
{
    init_fake_stuff();
    sprintf(test_string, "%*.*s", 15, 15, "Initialize and Destroy Test");
    CU_ASSERT(DRV_NOT_INITIALIZED == display_stop_text());
    CU_ASSERT(DRV_NOT_INITIALIZED == display_start_text(&test_string[0]));
    CU_ASSERT(DRV_NOT_INITIALIZED == display_init(NULL, 5, 5, 5, 0, true));
    CU_ASSERT(DRV_NOT_INITIALIZED == display_init(wrapper_printf, 5, 5, 5, 0, true));
    fake_queue_create = 5;
    CU_ASSERT(DRV_NOT_INITIALIZED == display_init(wrapper_printf, 5, 5, 5, 0, true));
    fake_semaphore_create = 5;
    CU_ASSERT(DRV_NOT_INITIALIZED == display_init(wrapper_printf, 5, 5, 5, 0, true));
    fake_x_task_create = 1;

    CU_ASSERT(DRV_SUCCESS == display_init(wrapper_printf, 5, 5, 5, 3, true));
    display_destroy();
    CU_ASSERT(DRV_NOT_INITIALIZED == display_start_text(&test_string[0]));
}

void test_stop_text( void )
{
    init_fake_stuff();
    CU_ASSERT(DRV_NOT_INITIALIZED == display_stop_text());
    fake_queue_create = 5;
    fake_semaphore_create = 5;
    fake_x_task_create = 1;

    CU_ASSERT(DRV_SUCCESS == display_init(wrapper_printf, 5, 5, 5, 3, true));
    display_destroy();
}

void test_start_text( void )
{
    init_fake_stuff();
    sprintf(test_string, "%*.*s", NUM_CHARS_ON_LCD+5, NUM_CHARS_ON_LCD+5, "Start Text String.........");
    CU_ASSERT(DRV_NOT_INITIALIZED == display_start_text(&test_string[0]));
    fake_queue_create = 5;
    fake_semaphore_create = 5;
    fake_x_task_create = 1;
    CU_ASSERT(DRV_SUCCESS == display_init(wrapper_printf, 5, 5, 5, 3, true));
    CU_ASSERT(DRV_SUCCESS == display_start_text( test_string ));
    sprintf(test_string, "%*.*s", 400, 400, "Start Text String.........");
    CU_ASSERT(DRV_STRING_TO_LONG == display_start_text( test_string ) );
    
    display_destroy();
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Display Test", NULL, NULL );
    CU_add_test( *suite, "Test Init/Destroy   ", test_init_destroy );
    CU_add_test( *suite, "Test stop text      ", test_stop_text );
    CU_add_test( *suite, "Test start text     ", test_start_text );
}

int main( int argc, char *argv[] )
{
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
        }

        CU_cleanup_registry();
    }

    return CU_get_error();
}
