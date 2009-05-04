#ifndef DISPLAY_INTERNAL_H_
#define DISPLAY_INTERNAL_H_

#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

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
    size_t length;
    size_t display_offset;
    state_of_display_t state;
    char text[MAX_DISPLAY_LENGTH];
};

struct display_globals {
    text_print_fct text_print_fn;
    portTickType scroll_speed;
    portTickType pause_at_beginning_of_text;
    portTickType pause_at_end_of_text;
    size_t maximum_number_characters_to_display;
    size_t num_characters_to_shift;
    struct text_display_info text_info;
    int32_t valid;
    struct {
        xQueueHandle queue_handle;
        xSemaphoreHandle mutex_handle;
        xTaskHandle task_handle;
        uint16_t identifier;
    } os;
    bool repeat;
};

struct display_message {
    uint16_t identifier;
    display_action_t action;
};

#endif /* DISPLAY_INTERNAL_H_ */
