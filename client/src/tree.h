#ifndef EU_TREE_H
#define EU_TREE_H


struct tnode{
	//string for port and id: arguments need to be added to functions as well
	int pid;
	int latency;
	int upload;
	int child;
	int max;
	int joinable; // maybe used for waiting for non-responsive nodes instead of entirely dropping.

	struct tnode *children[10];
	struct tnode *parent;
}

// initialize
// Used by source to initialize the tree and create the first node.
// Returns: a pointer to the head of the tree.
// Calls: nodealloc
struct tnode *initialize(int latency, int upload, int pid);


// addpeer
// Used to add individual followers to the tree.
// Returns: nothing
// Calls: nodealloc, locate
void addpeer(struct tnode *tree, int latency, int upload, int pid);


// removepeer
// Used for a friendly quit.
// Returns: nothing
// Calls: locate
void removepeer(struct tnode *tree, int pid);


// movepeer
// Used to move a peer that complains about its parent.
// Returns: nothing
// Calls: locate
void movepeer(struct tnode *tree, int pid);    // also return new parent


// locate
// Used to find next open spot in the tree
// Returns: pointer to parent with an open spot
// Calls: none
struct tnode *locate(struct tnode *tree);


// nodealloc
// Used to allocate a new node
// Returns: a pointer to the new node.
// Calls: malloc
struct tnode *nodealloc();


#endif
