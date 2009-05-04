#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "handle_msg_action.h"

#define LED_LENGTH   13
#define MAX_LENGTH   100
#define SCROLL_SPD   5
#define PAUSE_BEGIN  10
#define PAUSE_END    15

struct display_globals gld;
struct display_message msg;
portTickType ticks_to_wait_for_msg;
portTickType ticks_to_wait_before_looping;
char buffer[MAX_LENGTH];

size_t fake_print( char * string)
{
    int32_t num;
    char local_string[MAX_LENGTH];
        
    num = sprintf(local_string, "%.*s", LED_LENGTH, string);
    printf("%.*s   ", LED_LENGTH, string);
    if( num > -1 ) {
        return (size_t)num;
    }
    return 0;
}

void setup_gld_struct( char * string )
{
    gld.text_info.length = strlen(string);
    gld.text_info.display_offset = 0;
    gld.pause_at_beginning_of_text = PAUSE_BEGIN;
    gld.pause_at_end_of_text = PAUSE_END;
    gld.scroll_speed = SCROLL_SPD;
    gld.num_characters_to_shift = LED_LENGTH;
    strcpy( gld.text_info.text, string );
}


void test_unknown_action( void )
{
    ticks_to_wait_before_looping = 0;
    ticks_to_wait_for_msg = 0;
    memset(&gld, 0, sizeof(gld));
    memset(&msg, 0, sizeof(msg));
    msg.action = 100;
    CU_ASSERT( false == handle_msg_action(&msg, &ticks_to_wait_for_msg, &ticks_to_wait_before_looping, "", &gld) );
    CU_ASSERT( SOD_NOT_DISPLAYING == gld.text_info.state );
    CU_ASSERT( 0 == ticks_to_wait_before_looping );
    CU_ASSERT( portMAX_DELAY == ticks_to_wait_for_msg );
}

void test_stop_action( void )
{
    ticks_to_wait_before_looping = 0;
    ticks_to_wait_for_msg = 0;
    memset(&gld, 0, sizeof(gld));
    memset(&msg, 0, sizeof(msg));
    msg.action = DA_STOP;
    CU_ASSERT( false == handle_msg_action(&msg, &ticks_to_wait_for_msg, &ticks_to_wait_before_looping, "", &gld) );
    CU_ASSERT( SOD_NOT_DISPLAYING == gld.text_info.state );
    CU_ASSERT( 0 == ticks_to_wait_before_looping );
    CU_ASSERT( portMAX_DELAY == ticks_to_wait_for_msg );
}

void test_start_action( void )
{
    ticks_to_wait_before_looping = 0;
    ticks_to_wait_for_msg = 0;
    memset(&gld, 0, sizeof(gld));
    memset(&msg, 0, sizeof(msg));
    setup_gld_struct("Normal String Length");
    msg.action = DA_START;
    gld.text_print_fn = fake_print;
    CU_ASSERT( true == handle_msg_action(&msg, &ticks_to_wait_for_msg, &ticks_to_wait_before_looping, gld.text_info.text, &gld) );
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
    CU_ASSERT( PAUSE_BEGIN == ticks_to_wait_before_looping );
    CU_ASSERT( 0 == ticks_to_wait_for_msg );
    CU_ASSERT( gld.num_characters_to_shift == gld.text_info.display_offset );
}

void test_start_action_short_string( void )
{
    ticks_to_wait_before_looping = 0;
    ticks_to_wait_for_msg = 0;
    memset(&gld, 0, sizeof(gld));
    memset(&msg, 0, sizeof(msg));
    setup_gld_struct("Short");
    msg.action = DA_START;
    gld.text_print_fn = fake_print;
    CU_ASSERT( true == handle_msg_action(&msg, &ticks_to_wait_for_msg, &ticks_to_wait_before_looping, gld.text_info.text, &gld) );
    CU_ASSERT( SOD_NO_SCROLLING_NEEDED == gld.text_info.state );
    CU_ASSERT( SCROLL_SPD == ticks_to_wait_before_looping );
    CU_ASSERT( 0 == ticks_to_wait_for_msg );
}

void test_start_action_slow_scroll( void )
{
    ticks_to_wait_before_looping = 0;
    ticks_to_wait_for_msg = 0;
    memset(&gld, 0, sizeof(gld));
    memset(&msg, 0, sizeof(msg));
    setup_gld_struct("Normal String Length");
    gld.num_characters_to_shift = 1;
    msg.action = DA_START;
    gld.text_print_fn = fake_print;
    CU_ASSERT( true == handle_msg_action(&msg, &ticks_to_wait_for_msg, &ticks_to_wait_before_looping, gld.text_info.text, &gld) );
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
    CU_ASSERT( PAUSE_BEGIN == ticks_to_wait_before_looping );
    CU_ASSERT( 0 == ticks_to_wait_for_msg );
    CU_ASSERT( gld.num_characters_to_shift == gld.text_info.display_offset );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Handle Display Update", NULL, NULL );
    CU_add_test( *suite, "Unkown Action ", test_unknown_action);
    CU_add_test( *suite, "Stop Action ", test_stop_action);
    CU_add_test( *suite, "Start Action, Longer Title", test_start_action);
    CU_add_test( *suite, "Start Action, Short Title", test_start_action_short_string);
    CU_add_test( *suite, "Start Action, Longer Title, Slow Scroll", test_start_action_slow_scroll);
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
