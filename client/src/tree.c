#include "tree.h"
#include "alpha_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct tnode *initialize(int upload, char pid[], char addr[]){
	
	// create new node
	struct tnode *tree = nodealloc();

	// set node variables
	tree->upload = upload;
	tree->child = 0;
	tree->latency = 0;
	tree->max = EU_MAX_CHILD;
	//tree->joinable = 1;
	strcpy(tree->pid, pid);
	strcpy(tree->addr, addr);

	// return pointer to tree
	return tree;
}

void addpeer(struct tnode *tree, int latency, int upload, char pid[], char addr[]){

	struct tnode *parent;
	struct tnode *new;

	// locate a parent for this peer
	parent = locateEmpty(tree);

	// create a new node
	new = nodealloc();

	// set node variables
	new->latency = latency;
	new->upload = upload;
	new->child = 0;
	new->max = EU_MAX_CHILD;
	new->parent = parent;
	strcpy(new->pid, pid);
	strcpy(new->addr, addr);

	// add to children of parent
	parent->children[parent->child] = new;
	parent->child += 1;

	// todo: SEND APPROPRIATE MESSAGES
}

void removePeer(struct tnode *tree, char pid[]){

	struct tnode *remove;

	// find node to be removed in tree
	remove = findPeer(tree, pid);

	if(remove == NULL){
		fprintf(stderr, "tree: Could not find peer:\n");
		exit(1);
	}



	// move all the children
	while(remove->child > 0) {
		movePeer(tree, remove->children[remove->child-1]->pid);
	}

	// clear this node from its parent
	clearParent(remove);

	// free node memory
	free(remove);

	// todo: send messages
}

void movePeer(struct tnode *tree, char pid[]){
	
	struct tnode *newparent;
	struct tnode *move;
	struct tnode *oldparent;

	// find node to be moved
	move = findPeer(tree, pid);

	if( move == NULL){
		fprintf(stderr, "tree: Could not find peer:\n");
		exit(1);
	}

	// save old parent pointer
	oldparent = move->parent;

	// set old parents max
	oldparent->max = oldparent->child-1;

	// clear this node from its parent
	clearParent(move);

	// find a new spot for it
	newparent = locateEmpty(tree);

	newparent->children[newparent->child++] = move;
	move->parent = newparent;


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
			parent->child--; // decrement # of children
		}
	}
	
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

	//todo: in the case of no empty spots, this will loop forever.
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

struct tnode *findPeer(struct tnode *tree, char pid[]){

	struct tnode *find = NULL;
	struct alpha_queue *queue;
	struct tnode *temp;

	//create a queue and push source node
	queue = alpha_queue_new();

	if( alpha_queue_push( queue, (void*)tree) == false){
		fprintf(stderr, "tree: queue out of memory\n");
		exit(1);
	}

	//todo: loops forever if node doesn't exist in tree - should be fixed now with new while statement
	do{
		temp = (struct tnode*)alpha_queue_pop(queue); // pop an item and see if its the node you are looking for

		if(strcmp(pid, temp->pid) == 0)
			find = temp;
		else{	// add its children if its incorrent node
			int i;
			for ( i = 0; i < temp->child; i++){
				if( alpha_queue_push( queue, (void*)temp->children[i]) == false){
					fprintf(stderr, "tree: queue out of memory\n");
					exit(1);
				}
			}
		}
	}while(find == NULL && temp != NULL);

	alpha_queue_free(queue);	//free the queue
	
	return find;
}

struct tnode *nodealloc(){
	struct tnode *temp = NULL;

	// allocate new node
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

void freeTree(struct tnode *tree){

	int i;
	for ( i = 0; i < tree->child; i++)
		freeTree(tree->children[i]);

	free(tree);
}






//debug functions
void printTree(struct tnode *tree){
	// print tree recursively (depth first search)
	struct alpha_queue *queue;
	struct tnode *print;
	
	queue = alpha_queue_new();
	
	alpha_queue_push ( queue, (void*) tree);

	while(( print = (struct tnode*)alpha_queue_pop(queue)) != NULL) {
		printf("Node PID: %s\n", print->pid);
		printf("Node IP: %s\n", print->addr);
		printf("Node # of children: %d\n", print->child);
		if(print->parent != NULL)
			printf("Node Parent: %s\n", print->parent->pid);
		int i;
		for( i = 0; i < print->child; i++){
			printf("Node Child: %s\n", print->children[i]->pid);
			alpha_queue_push ( queue, (void*) print->children[i]);
		}
		printf("\n\n\n");

	}
}










