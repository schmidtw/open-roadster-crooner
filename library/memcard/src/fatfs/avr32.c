/*
 * Copyright (c) 2009  Weston Schmidt
 */

#include <stddef.h>

#include "../memcard.h"
#include "../block.h"
#include "diskio.h"

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
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
DSTATUS disk_initialize( BYTE drv )
{
    if( 0 != drv ) {
        return STA_NOINIT;
    }

    if( MC_CARD__MOUNTED == mc_get_status() ) {
        return STA_PROTECT;
    }

    return STA_NODISK;
}

DSTATUS disk_status( BYTE drv )
{
    if( 0 != drv ) {
        return STA_NOINIT;
    }

    return STA_PROTECT;
}

DRESULT disk_read( BYTE drv, BYTE *buf, DWORD lba, BYTE sector_count )
{
    if( (0 != drv) || (NULL == buf) || (0 == sector_count) ) {
        return RES_PARERR;
    }

    while( 0 < sector_count ) {
        if( MC_RETURN_OK != block_read(lba, buf) ) {
            return RES_ERROR;
        }
        sector_count--;
        lba += 512;
        buf += 512;
    }

    return RES_OK;
}

DRESULT disk_ioctl( BYTE drv, BYTE ctrl, void *buf )
{
    if( 0 != drv ) {
        return RES_PARERR;
    }
    switch( ctrl ) {
        case GET_SECTOR_COUNT:
        {
            uint32_t blocks;
            if( MC_RETURN_OK == mc_get_block_count(&blocks) ) {
                return RES_ERROR;
            }
            *((DWORD*) buf) = (DWORD) blocks;
            break;
        }

        case GET_SECTOR_SIZE:
        {
            *((WORD*) buf) = 512;
            break;
        }

        case GET_BLOCK_SIZE:
        {
            *((DWORD*) buf) = 512;
            break;
        }

        default:
            return RES_PARERR;
    }

    return RES_OK;
}

#if	_READONLY == 0
DRESULT disk_write( BYTE drv, const BYTE *buf, DWORD lba, BYTE sector_count )
{
    if( 0 != drv ) {
        return RES_PARERR;
    }
    return RES_PARERR;
}
#endif

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
