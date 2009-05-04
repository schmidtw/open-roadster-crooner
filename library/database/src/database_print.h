#ifndef __DATABASE_PRINT_H__
#define __DATABASE_PRINT_H__

void database_print( void );

/**
 * Displays the <type>_node and all sub-nodes
 * 
 * @param spaces Number of blank spaces to be printed before
 *        the name
 */
void group_print( group_node_t * group, int spaces );
void artist_print( artist_node_t * artist, int spaces );
void album_print( album_node_t * album, int spaces );
void song_print( song_node_t * song, int spaces );

#endif /* __DATABASE_PRINT_H__ */
