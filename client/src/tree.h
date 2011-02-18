#ifndef EU_TREE_H
#define EU_TREE_H


struct tree_t;


/*
 * Used by source to initialize the tree.
 * Returns a pointer to the tree or NULL if out of memory.
 */
struct tree_t* 
initialize(int streambw, int peerbw, char pid[], char addr[], uint16_t port);


/*
 * Used to add followers to the tree.
 * Returns  0 if peer was added.
 * Returns -1 if memory error.
 * Returns -2 if no empty slots found.
 */
int 
addPeer(struct tree_t *tree, int peerbw, char pid[], char addr[], uint16_t port);


/*
 * Used for a friendly quit.
 * Returns  0 if peer was removed.
 * Returns -1 if memory error.
 */
int 
removePeer(struct tree_t *tree, char pid[]);


/*
 * Used to move a peer away from its current parent.
 * Returns  0 if peer was moved successfully.
 * Returns -1 if memory error.
 * Returns -2 if peer not found in tree.
 * Returns -3 if no empty slots found.
 */
int
movePeer(struct tree_t *tree, char pid[]);


/*
 * Used to free a tree structure
 */
void
freeTree(struct tree_t *tree);


//debug function
void printTree(struct tree_t *tree);


#endif
