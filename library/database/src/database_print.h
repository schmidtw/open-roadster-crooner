#ifndef __DATABASE_PRINT_H__
#define __DATABASE_PRINT_H__

void database_print( void );

/**
 * Displays the <type>_node and all sub-nodes
 * 
 * @param spaces Number of blank spaces to be printed before
 *        the name
 */
void artist_print( generic_node_t * artist, int spaces );
void album_print( generic_node_t * album, int spaces );
void song_print( generic_node_t * song, int spaces );

#endif /* __DATABASE_PRINT_H__ */
