#ifndef __IBUS_H__
#define __IBUS_H__

#include <stdint.h>

#include <freertos/queue.h>

#include "messages.h"

/* The constants are used by the ibus spec, so don't change them */

typedef enum {
    IBUS_RX_CMD__STATUS         = 0x00,
    IBUS_RX_CMD__STOP           = 0x01,
    IBUS_RX_CMD__PAUSE          = 0x02,
    IBUS_RX_CMD__PLAY           = 0x03,
    IBUS_RX_CMD__FAST_PLAY      = 0x04,
    IBUS_RX_CMD__SEEK           = 0x05,
    IBUS_RX_CMD__CHANGE_DISC    = 0x06,
    IBUS_RX_CMD__SCAN_DISC      = 0x07,
    IBUS_RX_CMD__RANDOMIZE      = 0x08,

    IBUS_RX_CMD__POLL
} ibus_rx_cmd_t;

typedef enum {
    IBUS_TX_CMD__STOPPED            = 0x00,
    IBUS_TX_CMD__PAUSED             = 0x01,
    IBUS_TX_CMD__PLAYING            = 0x02,
    IBUS_TX_CMD__FAST_PLAYING       = 0x03,
    IBUS_TX_CMD__REWINDING          = 0x04,
    IBUS_TX_CMD__SEEKING_NEXT       = 0x05,
    IBUS_TX_CMD__SEEKING_PREV       = 0x06,
    IBUS_TX_CMD__SEEKING            = 0x07,
    IBUS_TX_CMD__LOADING_DISC       = 0x08,
    IBUS_TX_CMD__CHECKING_FOR_DISC  = 0x09,

    IBUS_TX_CMD__ANNOUNCE,
    IBUS_TX_CMD__POLL_RESPONSE
} ibus_tx_cmd_t;

typedef enum {
    IBUS__OFF   = 0x00,
    IBUS__ON    = 0x01
} ibus_on_off_t;

typedef enum {
    IBUS__NEXT  = 0x00,
    IBUS__PREV  = 0x01
} ibus_direction_t;

typedef enum {
    IBUS_AUDIO_STATE__STOPPED = 0x02,
    IBUS_AUDIO_STATE__PLAYING = 0x09,
    IBUS_AUDIO_STATE__PAUSED  = 0x0c
} ibus_audio_state_t;

typedef struct {
    message_type_t type;

    ibus_rx_cmd_t cmd;

    union {
        uint8_t disc;               /* Only [1-6] is valid */
        ibus_on_off_t scan_disc;
        ibus_on_off_t randomize;
        ibus_direction_t seek;
    } d;
} ibus_rx_message_t;

#define IBUS_DISC_STATUS__MAGAZINE_PRESENT  0x80
#define IBUS_DISC_STATUS__DISC_ANY          0x3f
#define IBUS_DISC_STATUS__DISC_1            0x01
#define IBUS_DISC_STATUS__DISC_2            0x02
#define IBUS_DISC_STATUS__DISC_3            0x04
#define IBUS_DISC_STATUS__DISC_4            0x08
#define IBUS_DISC_STATUS__DISC_5            0x10
#define IBUS_DISC_STATUS__DISC_6            0x20
#define IBUS_TEXT_LENGTH_MAX                12

typedef struct {
    ibus_audio_state_t audio_state;
    uint8_t disc_status;
    uint8_t current_disc;
    uint8_t current_track;
} ibus_status_msg_t;

typedef struct {
    ibus_audio_state_t audio_state;
    uint8_t disc;
    uint8_t mask;
    bool last_failed;
} ibus_disc_check_t;

typedef struct {
    message_type_t type;

    ibus_tx_cmd_t cmd;

    union {
        ibus_status_msg_t status;
        ibus_disc_check_t check_for_disc;
        char msg_text[IBUS_TEXT_LENGTH_MAX + 1];
    } d;
} ibus_tx_message_t;

/**
 *  Used to initialize the iBus service.
 */
void ibus_init( void );

/**
 *  Used to register with the iBus service.
 *
 *  @param incoming where to receive asynchronous messages
 *  @param outgoing where to post the response message
 *
 *  @return 0 on success
 */
int32_t ibus_register( xQueueHandle q );

/**
 *  Used to de-register with this service.
 *
 *  @param incoming where to receive asynchronous messages
 *
 *  @return 0 on success
 */
int32_t ibus_deregister( xQueueHandle q );

/**
 *  Used to get a response message to send to the iBus service.
 *
 *  @note blocks until success
 *
 *  @return the pointer to the message
 */
ibus_tx_message_t *ibus_message_alloc( void );

/**
 *  Used to free either an allocated ibus_response_t message
 *  or an asynchronous message.
 *
 *  @param msg the message to free
 */
void ibus_message_free( void *msg );

/**
 *  Used to post a message to the ibus queue.  The memory MUST
 *  be gotten from ibus_message_alloc().
 *
 *  @note blocks until success
 *
 *  @param msg the message to post
 */
void ibus_message_post( ibus_tx_message_t *msg );

/**
 *  Used to send a text string to the radio for display.
 *  
 *  @param text NULL terminated string which is to be displayed
 *         to the radio
 *  
 *  @return number of characters displayed
 */
size_t ibus_print( char * text );
#endif
