#ifndef EU_TREE_H
#define EU_TREE_H


#define EU_MAX_CHILD 10

struct tnode{
	//string for port and id: arguments need to be added to functions as well
	int pid;
	int latency;
	int upload;
	int child;
	int max;
	int joinable; // maybe used for waiting for non-responsive nodes instead of entirely dropping.

	struct tnode *children[EU_MAX_CHILD];
	struct tnode *parent;
};


/*
 * Used by source to initialize the tree.
 * Returns a pointer to the tree head.
 */
struct tnode* 
initialize(int upload, int pid);


/*
 * Used to add followers to the tree.
 */
void 
addPeer(struct tnode *tree, int latency, int upload, int pid);


/*
 * Used for a friendly quit.
 */
void 
removePeer(struct tnode *tree, int pid);


/*
 * Used to move a peer away from its current parent.
 */
void 
movePeer(struct tnode *tree, int pid);    // also return new parent


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
findPeer(struct tnode *tree, int pid);


/*
 * Used to allocate a new node.
 * Returns a pointer to the new node.
 */
struct tnode* 
nodealloc();


#endif
