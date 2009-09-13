/*
 * Copyright (c) 2009  Weston Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdbool.h>
#include <stdint.h>

#include <led/led.h>
#include <freertos/semphr.h>

#include "device-status.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* Yellow fading */
static const led_state_t __no_radio[] = {
    { .red = 0x00, .green = 0x00, .blue = 0x00, .duration = 100 },

    { .red = 0x0f, .green = 0x0f, .blue = 0x00, .duration = 31  },
    { .red = 0x2e, .green = 0x2e, .blue = 0x00, .duration = 31  },
    { .red = 0x4d, .green = 0x4d, .blue = 0x00, .duration = 31  },
    { .red = 0x6d, .green = 0x6d, .blue = 0x00, .duration = 31  },
    { .red = 0x8d, .green = 0x8d, .blue = 0x00, .duration = 31  },
    { .red = 0xac, .green = 0xab, .blue = 0x00, .duration = 31  },
    { .red = 0xcb, .green = 0xcb, .blue = 0x00, .duration = 31  },
    { .red = 0xea, .green = 0xeb, .blue = 0x00, .duration = 31  },

    { .red = 0xff, .green = 0xff, .blue = 0x00, .duration = 250 },

    { .red = 0xea, .green = 0xeb, .blue = 0x00, .duration = 31  },
    { .red = 0xcb, .green = 0xcb, .blue = 0x00, .duration = 31  },
    { .red = 0xac, .green = 0xab, .blue = 0x00, .duration = 31  },
    { .red = 0x8d, .green = 0x8d, .blue = 0x00, .duration = 31  },
    { .red = 0x6d, .green = 0x6d, .blue = 0x00, .duration = 31  },
    { .red = 0x4d, .green = 0x4d, .blue = 0x00, .duration = 31  },
    { .red = 0x2e, .green = 0x2e, .blue = 0x00, .duration = 31  },
    { .red = 0x0f, .green = 0x0f, .blue = 0x00, .duration = 31  }
};

/* Orange / Blue toggling */
static const led_state_t __card_being_scanned[] = {
    { .red = 0xff, .green = 0x45, .blue = 0x00, .duration = 100 },
    { .red = 0x00, .green = 0x00, .blue = 0xff, .duration = 100 }
};

/* Red fading */
static const led_state_t __card_unusable[] = {
    { .red = 0x00, .green = 0x00, .blue = 0x00, .duration = 100 },

    { .red = 0x0f, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x2e, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x4d, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x6d, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x8d, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0xac, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0xcb, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0xea, .green = 0x00, .blue = 0x00, .duration = 31  },

    { .red = 0xff, .green = 0x00, .blue = 0x00, .duration = 250 },

    { .red = 0xea, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0xcb, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0xac, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x8d, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x6d, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x4d, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x2e, .green = 0x00, .blue = 0x00, .duration = 31  },
    { .red = 0x0f, .green = 0x00, .blue = 0x00, .duration = 31  }
};

/* Green fading */
static const led_state_t __normal[] = {
    { .red = 0x00, .green = 0x00, .blue = 0x00, .duration = 250 },

    { .red = 0x00, .green = 0x13, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x31, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x4f, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x6e, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x8d, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0xac, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0xca, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0xe9, .blue = 0x00, .duration = 31  },

    { .red = 0x00, .green = 0xff, .blue = 0x00, .duration = 500 },

    { .red = 0x00, .green = 0xe9, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0xca, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0xac, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x8d, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x6e, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x4f, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x31, .blue = 0x00, .duration = 31  },
    { .red = 0x00, .green = 0x13, .blue = 0x00, .duration = 31  }
};

static xSemaphoreHandle __mutex;
static device_status_t __current;


/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See device-status.h for details. */
void device_status_init( void )
{
    __mutex = xSemaphoreCreateMutex();
    __current = DS__NO_RADIO_CONNECTION;
}

/* See device-status.h for details. */
void device_status_set( const device_status_t status )
{
    size_t size;
    const led_state_t *cmd;

    /* Don't send the same status that we currently are. */
    xSemaphoreTake( __mutex, portMAX_DELAY );
    if( __current == status ) {
        xSemaphoreGive( __mutex );
        return;
    }

    switch( status ) {
        case DS__NO_RADIO_CONNECTION:
            cmd = __no_radio;
            size = sizeof(__no_radio) / sizeof(led_state_t);
            break;

        case DS__CARD_BEING_SCANNED:
            cmd = __card_being_scanned;
            size = sizeof(__card_being_scanned) / sizeof(led_state_t);
            break;

        case DS__CARD_UNUSABLE:
            cmd = __card_unusable;
            size = sizeof(__card_unusable) / sizeof(led_state_t);
            break;

        case DS__NORMAL:
            cmd = __normal;
            size = sizeof(__normal) / sizeof(led_state_t);
            break;

        default:
            xSemaphoreGive( __mutex );
            return;
    }

    __current = status;
    led_set_state( (led_state_t *) cmd, size, true, NULL );
    xSemaphoreGive( __mutex );
}

/* See device-status.h for details. */
device_status_t device_status_get( void )
{
    return __current;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
