#ifndef EU_TREE_H
#define EU_TREE_H



#define EU_MAX_CHILD 3

struct tnode;


/*
 * Used by source to initialize the tree.
 * Returns a pointer to the tree head.
 */
struct tnode* 
initialize(int upload, char pid[], char addr[]);


/*
 * Used to add followers to the tree.
 */
void 
addPeer(struct tnode *tree, int latency, int upload, char pid[], char addr[]);


/*
 * Used for a friendly quit.
 */
void 
removePeer(struct tnode *tree, char pid[]);


/*
 * Used to move a peer away from its current parent.
 */
void 
movePeer(struct tnode *tree, char pid[]);


void printTree(struct tnode *tree);


#endif
