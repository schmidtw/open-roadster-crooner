#ifndef __MESSAGES_H__
#define __MESSAGES_H__

typedef enum {
    MT_IBUS__RX_MESSAGE   = 0x0000,
    MT_IBUS__TX_MESSAGE   = 0x0001,
    MT_IBUS__TEXT_MESSAGE = 0x0002,
    MT_DISC__STATUS       = 0x0010
} message_type_t;

#endif
