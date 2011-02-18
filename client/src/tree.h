#ifndef EU_TREE_H
#define EU_TREE_H


struct tree_t;


/*
 * Used by source to initialize the tree.
 * Returns a pointer to the tree head.
 */
struct tree_t* 
initialize(int streambw, int peerbw, char pid[], char addr[], uint16_t port);


/*
 * Used to add followers to the tree.
 */
void 
addPeer(struct tree_t *tree, int peerbw, char pid[], char addr[], uint16_t port);


/*
 * Used for a friendly quit.
 */
void 
removePeer(struct tree_t *tree, char pid[]);


/*
 * Used to move a peer away from its current parent.
 */
void 
movePeer(struct tree_t *tree, char pid[]);


/*
 * Used to free a tree structure
 */
void
freeTree(struct tree_t *tree);


//debug function
void printTree(struct tree_t *tree);


#endif
