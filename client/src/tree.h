#ifndef EU_TREE_H
#define EU_TREE_H


#include <netdb.h>


#define EU_ADDRSTRLEN INET6_ADDRSTRLEN
#define EU_TOKENSTRLEN 224
#define EU_MAX_CHILD 3

struct tnode{

	char pid[EU_TOKENSTRLEN];
	char addr[EU_ADDRSTRLEN];
	int latency;
	int upload;
	int child;
	int max;
	int joinable;

	struct tnode *children[EU_MAX_CHILD];
	struct tnode *parent;
};


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


/*
 * Used to detach a node from its parent. 
 */
void 
clearParent(struct tnode *remove);


/*
 * Used to find best open spot in the tree.
 * Returns a pointer to the future parent.
 */
struct tnode* 
locateEmpty(struct tnode *tree);


/*
 * Used to find a peer within a tree.
 * Returns a pointer to the peer, or NULL if it doesn't exist.
 */
struct tnode* 
findPeer(struct tnode *tree, char pid[]);


/*
 * Used to allocate a new node.
 * Returns a pointer to the new node.
 */
struct tnode* 
nodealloc();


/*
 * Used to free a tree's memory.
 */
void
freeTree(struct tnode *tree);

void printTree(struct tnode *tree);


#endif
