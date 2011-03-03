#ifndef EU_TREE_H
#define EU_TREE_H

#include <stdlib.h>
#include <stdint.h>

struct tree_t;


/*
 * Used by source to initialize the tree.
 * Returns a pointer to the tree or NULL if out of memory.
 */
struct tree_t* 
initialize(void* socket, int streambw, int peerbw, char pid[], char addr[], char port[], int debug_mode);


/*
 * Used to add followers to the tree.
 * Returns  0 if peer was added.
 * Returns -1 if memory error.
 * Returns -2 if no empty slots found and could not switch with a leaf, or getLeaf had error (should not happen)
 */
int 
addPeer(struct tree_t *tree, int peerbw, char pid[], char addr[], char port[]);


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
 * Returns -3 if no empty slots found and trying to move a leaf
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

void* getSocket(struct tree_t *tree);

uint64_t countRootChildren(struct tree_t *tree);

struct peer_info *getRootChild(struct tree_t *tree, uint64_t pos);



#endif
