#ifndef DISPLAY_INTERNAL_H_
#define DISPLAY_INTERNAL_H_

#include <time.h>

#include "freertos/os.h"

#include "display.h"

#define MAX_DISPLAY_LENGTH       260

typedef enum {
    SOD_NOT_DISPLAYING = 0,
    SOD_BEGINNING_OF_TEXT,
    SOD_MIDDLE_OF_TEXT,
    SOD_END_OF_TEXT,
    SOD_NO_SCROLLING_NEEDED
} state_of_display_t;

typedef enum {
    DA_START,
    DA_STOP
} display_action_t;

struct text_display_info {
    char * text;
    uint16_t identifier;
};

struct text_display_state {
    size_t length;
    size_t display_offset;
    state_of_display_t state;
    uint32_t next_draw_time;
    struct text_display_info text_info;
};


struct display_globals {
    text_print_fct text_print_fn;
    uint32_t scroll_speed;
    uint32_t pause_at_beginning_of_text;
    uint32_t pause_at_end_of_text;
    size_t maximum_number_characters_to_display;
    size_t num_characters_to_shift;
    struct text_display_state text_state;
    bool valid;
    struct {
        queue_handle_t queue_handle;
        semaphore_handle_t mutex_handle;
        task_handle_t task_handle;
    } os;
    bool repeat;
};

struct display_message {
    struct text_display_info text_info;
    display_action_t action;
};

#endif /* DISPLAY_INTERNAL_H_ */
