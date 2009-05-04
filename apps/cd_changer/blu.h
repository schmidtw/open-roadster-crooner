#ifndef __BLU_H__
#define __BLU_H__

#include "messages.h"

#define BLU_DISC_STATUS__NO_MAGAZINE       0x00
#define BLU_DISC_STATUS__MAGAZINE_PRESENT  0x80
#define BLU_DISC_STATUS__DISC_ANY          0x3f
#define BLU_DISC_STATUS__DISC_1            0x01
#define BLU_DISC_STATUS__DISC_2            0x02
#define BLU_DISC_STATUS__DISC_3            0x04
#define BLU_DISC_STATUS__DISC_4            0x08
#define BLU_DISC_STATUS__DISC_5            0x10
#define BLU_DISC_STATUS__DISC_6            0x20

typedef struct {
    message_type_t type;

    uint8_t disc_status;
} disc_status_message_t;

void blu_init( void );

disc_status_message_t *disc_status_message_alloc( void );
void blu_message_free( void *msg );

void blu_message_post( void *msg );
#endif
