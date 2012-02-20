#ifndef __INTERNAL_DATABASE_H__
#define __INTERNAL_DATABASE_H__

#include <stdbool.h>
#include "database.h"

typedef struct {
    generic_node_t *root;
    bool initialized;
} root_database_node_t;

/**
 * Reference to the root database node.
 */
extern root_database_node_t rdn;

#endif /* __INTERNAL_DATABASE_H__ */
