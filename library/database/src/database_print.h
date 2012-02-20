#ifndef __DATABASE_PRINT_H__
#define __DATABASE_PRINT_H__

#include <binary-tree-avl/binary-tree-avl.h>

void database_print( void );

/**
 * Displays the <type>_node and all sub-nodes
 * 
 * @param spaces Number of blank spaces to be printed before
 *        the name
 */
bt_ir_t artist_print(bt_node_t *node, volatile void *user_data);
bt_ir_t album_print(bt_node_t *node, volatile void *user_data);
bt_ir_t song_print(bt_node_t *node, volatile void *user_data);

#endif /* __DATABASE_PRINT_H__ */
