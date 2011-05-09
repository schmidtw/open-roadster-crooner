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
#define REDRAW_SPD   17

struct display_globals gld;
struct display_message msg;
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

void setup_gld_struct()
{
    gld.text_info.length = 0;
    gld.text_info.display_offset = 0;
    gld.pause_at_beginning_of_text = PAUSE_BEGIN;
    gld.pause_at_end_of_text = PAUSE_END;
    gld.scroll_speed = SCROLL_SPD;
    gld.redraw_no_scrolling = REDRAW_SPD;
    gld.num_characters_to_shift = LED_LENGTH;
    gld.text_info.text = "";
}


void test_unknown_action( void )
{
    bzero(&gld, sizeof(gld));
    bzero(&msg, sizeof(msg));
    msg.action = 100;
    handle_msg_action(&msg, &gld);
    CU_ASSERT( SOD_NOT_DISPLAYING == gld.text_info.state );
}

void test_stop_action( void )
{
    bzero(&gld, sizeof(gld));
    bzero(&msg, sizeof(msg));
    msg.action = DA_STOP;
    handle_msg_action(&msg, &gld);
    CU_ASSERT( SOD_NOT_DISPLAYING == gld.text_info.state );
    CU_ASSERT( 0 == gld.text_info.next_draw_time );
}

void test_start_action( void )
{
    char * String = "Normal String Length";
    bzero(&gld, sizeof(gld));
    bzero(&msg, sizeof(msg));
    setup_gld_struct();
    msg.action = DA_START;
    msg.text = String;
    gld.text_print_fn = fake_print;
    handle_msg_action(&msg,  &gld);
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
    CU_ASSERT( PAUSE_BEGIN == gld.text_info.next_draw_time );
    CU_ASSERT( gld.num_characters_to_shift == gld.text_info.display_offset );
}

void test_start_action_short_string( void )
{
    char * String = "Short";
    bzero(&gld, sizeof(gld));
    bzero(&msg, sizeof(msg));
    setup_gld_struct( String );
    msg.action = DA_START;
    msg.text = String;
    gld.text_print_fn = fake_print;
    handle_msg_action(&msg, &gld);
    CU_ASSERT( SOD_NO_SCROLLING_NEEDED == gld.text_info.state );
    CU_ASSERT( REDRAW_SPD == gld.text_info.next_draw_time );
}

void test_start_action_slow_scroll( void )
{
    char * String = "Normal String Length";
    bzero(&gld, sizeof(gld));
    bzero(&msg, sizeof(msg));
    setup_gld_struct();
    msg.action = DA_START;
    msg.text = String;
    gld.num_characters_to_shift = 1;
    gld.text_print_fn = fake_print;
    handle_msg_action(&msg, &gld);
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
    CU_ASSERT( PAUSE_BEGIN == gld.text_info.next_draw_time );
    CU_ASSERT( gld.num_characters_to_shift == gld.text_info.display_offset );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Handle Message Action", NULL, NULL );
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
