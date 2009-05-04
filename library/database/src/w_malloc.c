#include <stdlib.h>
#include <string.h>
#include "w_malloc.h"

void * w_malloc( size_t size )
{
    void * ptr;
    
    ptr = malloc(size);
    if( NULL != ptr ) {
        memset(ptr, '\0', size);
    }
    return ptr;
}
