#ifndef __INTERNAL_DATABASE_H__
#define __INTERNAL_DATABASE_H__

#include <stdbool.h>
#include <linked-list/linked-list.h>
#include "database.h"

#define MAX_NUMBER_GROUPS 6

typedef struct {
    openDir_fct openDir;
    getNext_fct openNext;
    fclose_fct fclose;
    fopen_fct fopen;
    
    ll_list_t groups;
    uint32_t size_list;
    bool initialized;
} root_database_node_t;

/**
 * Reference to the root database node.
 */
extern root_database_node_t rdn;

#endif /* __INTERNAL_DATABASE_H__ */
