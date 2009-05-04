#ifndef __GROUP_H__
#define __GROUP_H__

#include <linked-list/linked-list.h>

/**
 * @param name MUST not be NULL
 * 
 * @return properly setup ll_node_t * with name, or NULL if an error occurred
 */
ll_node_t * get_new_group_and_node( const char * name );

/**
 * Deletion function which should be referenced when using
 * the linked listed deleter function.
 */
void delete_group(ll_node_t *node, volatile void *user_data);

#endif /* __GROUP_H__ */
