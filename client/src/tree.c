#include "tree.h"
#include "alpha_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct tnode *initialize(int upload, int pid){
	
	// create new node
	struct tnode *tree = nodealloc();

	// set node variables
	tree->upload = upload;
	tree->pid = pid;
	tree->child = 0;
	tree->latency = 0;
	tree->max = EU_MAX_CHILD;
	tree->joinable = 1;

	// return pointer to tree
	return tree;
}

void addpeer(struct tnode *tree, int latency, int upload, int pid){

	struct tnode *parent;
	struct tnode *new;

	// locate a parent for this peer
	parent = locateEmpty(tree);

	// create a new node
	new = nodealloc();

	// set node variables
	new->latency = latency;
	new->upload = upload;
	new->pid = pid;
	new->child = 0;
	new->max = EU_MAX_CHILD;
	new->joinable = 1;
	new->parent = parent;

	// add to children of parent
	parent->children[parent->child] = new;
	parent->child += 1;

	// todo: SEND APPROPRIATE MESSAGES
}

void removePeer(struct tnode *tree, int pid){

	struct tnode *remove;

	// find node to be removed in tree
	remove = findPeer(tree, pid);

	if(remove == NULL){
		fprintf(stderr, "tree: Could not find peer:\n");
		exit(1);
	}

	// clear this node from its parent
	clearParent(remove);

	// move all the children
	while(remove->child > 0) {
		movePeer(tree, remove->children[remove->child]->pid);
	}

	// free node memory
	free(remove);

	// todo: send messages
}

void movePeer(struct tnode *tree, int pid){
	
	struct tnode *newparent;
	struct tnode *move;
	struct tnode *oldparent;

	// find node to be moved
	move = findPeer(tree, pid);

	if( remove == NULL){
		fprintf(stderr, "tree: Could not find peer:\n");
		exit(1);
	}

	// save old parent pointer and set to non-joinable
	oldparent = move->parent;
	oldparent->joinable = 0;

	// set old parents max
	oldparent->max = oldparent->child-1;

	// clear this node from its parent
	clearParent(move);

	// find a new spot for it
	newparent = locateEmpty(tree);

	newparent->children[newparent->child++] = move;

	// set old parent back to joinable
	oldparent->joinable = 1;

	// todo: send messages
}

void clearParent(struct tnode *remove){
	// get parent
	struct tnode *parent = remove->parent;

	int i;
	for( i = 0; i < parent->child; i++){
		// find child in parent
		if( remove == parent->children[i]){
			// shift over other children
			for( ; i < parent->child-1; i++){
				parent->children[i] = parent->children[i+1];
			}
			parent->children[i] = NULL; // set last position to null
		}
	}
	parent->child--; // decrement # of children
}

struct tnode *locateEmpty(struct tnode *tree){
	
	struct tnode *find = NULL;
	struct alpha_queue *queue;
	struct tnode *temp;

	queue = alpha_queue_new();

	if( alpha_queue_push( queue, (void*)tree) == false){
		fprintf(stderr, "tree: queue out of memory\n");
		exit(1);
	}

	while(find == NULL) {
		temp = (struct tnode*)alpha_queue_pop(queue);

		if(temp->child < temp->max)
			find = temp;
		else{
			int i;
			for ( i = 0; i < temp->child; i++){
				if( alpha_queue_push( queue, (void*)temp->children[i]) == false){
					fprintf(stderr, "tree: queue out of memory\n");
					exit(1);
				}
			}
		}
	}

	alpha_queue_free(queue);
	
	return find;

}

struct tnode *findPeer(struct tnode *tree, int pid){

	struct tnode *find = NULL;
	struct alpha_queue *queue;
	struct tnode *temp;


	queue = alpha_queue_new();

	if( alpha_queue_push( queue, (void*)tree) == false){
		fprintf(stderr, "tree: queue out of memory\n");
		exit(1);
	}


	do{
		temp = (struct tnode*)alpha_queue_pop(queue);

		if(pid == temp->pid)
			find = temp;
		else{
			int i;
			for ( i = 0; i < temp->child; i++){
				if( alpha_queue_push( queue, (void*)temp->children[i]) == false){
					fprintf(stderr, "tree: queue out of memory\n");
					exit(1);
				}
			}
		}
	}while(temp == NULL);

	alpha_queue_free(queue);
	
	return find;
}

struct tnode *nodealloc(){
	struct tnode *temp = NULL;

	// malloc new node, check for out of memory
	if(( temp = (struct tnode*)malloc(sizeof(struct tnode))) == NULL) {
		fprintf(stderr, "tree: out of memory\n");
		exit(1);
	}

	// initilize internal pointers to NULL
	temp->parent = NULL;
	int i;
	for( i = 0; i < EU_MAX_CHILD; i++){
		temp->children[i] = NULL;
	}

	return temp;
}
