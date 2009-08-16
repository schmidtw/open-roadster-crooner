#ifndef __ID3_H__
#define __ID3_H__

#include <fatfs/ff.h>
#include "metadata.h"

void get_mp3_metadata( FIL *file, struct mp3entry *entry );

#endif
