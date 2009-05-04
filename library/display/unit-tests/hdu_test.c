#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>

#include "handle_display_update.h"

#define LED_LENGTH   13
#define MAX_LENGTH   100
#define SCROLL_SPD   5
#define PAUSE_BEGIN  10
#define PAUSE_END    15

struct display_globals gld;
portTickType ticks_to_wait;
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
}


void test_not_displaying( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_NOT_DISPLAYING;
    handle_display_update(&ticks_to_wait, "", &gld );
    CU_ASSERT( SOD_NOT_DISPLAYING == gld.text_info.state );
    CU_ASSERT( 0 == ticks_to_wait );
}

void test_beginning_text( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_BEGINNING_OF_TEXT;
    gld.text_print_fn = fake_print;
    sprintf(buffer, "Normal Size Title");
    setup_gld_struct(buffer);
    handle_display_update( &ticks_to_wait, buffer, &gld );
    CU_ASSERT( SCROLL_SPD == ticks_to_wait );
    CU_ASSERT( LED_LENGTH == gld.text_info.display_offset );
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
}

void test_middle_text( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_MIDDLE_OF_TEXT;
    gld.text_print_fn = fake_print;
    sprintf(buffer, "Normal Size Title");
    setup_gld_struct(buffer);
    gld.text_info.display_offset = 1;
    handle_display_update( &ticks_to_wait, buffer, &gld );
    CU_ASSERT( SCROLL_SPD == ticks_to_wait );
    CU_ASSERT( (LED_LENGTH+1) == gld.text_info.display_offset );
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
}

void test_end_text( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_MIDDLE_OF_TEXT;
    gld.text_print_fn = fake_print;
    sprintf(buffer, "Normal Size Title");
    setup_gld_struct(buffer);
    gld.text_info.display_offset = LED_LENGTH;
    handle_display_update( &ticks_to_wait, buffer, &gld );
    CU_ASSERT( PAUSE_END == ticks_to_wait );
    CU_ASSERT( SOD_END_OF_TEXT == gld.text_info.state );
}

void test_restart_text( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_END_OF_TEXT;
    gld.text_print_fn = fake_print;
    sprintf(buffer, "Normal Size Title");
    setup_gld_struct(buffer);
    gld.text_info.display_offset = gld.text_info.length;
    handle_display_update( &ticks_to_wait, buffer, &gld );
    CU_ASSERT( PAUSE_BEGIN == ticks_to_wait );
    CU_ASSERT( SOD_BEGINNING_OF_TEXT == gld.text_info.state );
}

void test_no_scrolling_text( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_NO_SCROLLING_NEEDED;
    gld.text_print_fn = fake_print;
    sprintf(buffer, "Short");
    setup_gld_struct(buffer);
    handle_display_update( &ticks_to_wait, buffer, &gld );
    CU_ASSERT( SCROLL_SPD == ticks_to_wait );
    CU_ASSERT( 0 == gld.text_info.display_offset );
    CU_ASSERT( SOD_NO_SCROLLING_NEEDED == gld.text_info.state );
}

void test_normal_scrolling_middle( void )
{
    ticks_to_wait = 0;
    memset(&gld, 0, sizeof(gld));
    gld.text_info.state = SOD_MIDDLE_OF_TEXT;
    gld.text_print_fn = fake_print;
    sprintf(buffer, "Normal Size Title");
    setup_gld_struct(buffer);
    gld.num_characters_to_shift = 1;
    gld.text_info.display_offset = 1;
    handle_display_update( &ticks_to_wait, buffer, &gld );
    CU_ASSERT( SCROLL_SPD == ticks_to_wait );
    CU_ASSERT( 2 == gld.text_info.display_offset );
    CU_ASSERT( SOD_MIDDLE_OF_TEXT == gld.text_info.state );
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "Handle Display Update", NULL, NULL );
    CU_add_test( *suite, "Not Displaying ", test_not_displaying);
    CU_add_test( *suite, "Begining Text  ", test_beginning_text);
    CU_add_test( *suite, "Middle Text  ", test_middle_text);
    CU_add_test( *suite, "End Text  ", test_end_text);
    CU_add_test( *suite, "Restart Text  ", test_restart_text);
    CU_add_test( *suite, "No Scrolling Text  ", test_no_scrolling_text);
    CU_add_test( *suite, "Normal Scrolling Text  ", test_normal_scrolling_middle);
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
