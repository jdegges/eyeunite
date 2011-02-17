#include "tree.h"
#include "alpha_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>

#define EU_ADDRSTRLEN INET6_ADDRSTRLEN
#define EU_TOKENSTRLEN 224

struct tnode{
  char pid[EU_TOKENSTRLEN];
  char addr[EU_ADDRSTRLEN];
  int latency;
  int upload;
  int max;
  int joinable; // maybe used for waiting for non-responsive nodes instead of entirely dropping.

  struct list *children;
  struct tnode *parent;
};

/*
* Used to allocate a new node.
* Returns a pointer to the new node.
*/
static struct tnode *nodealloc(void){
  struct tnode *temp = NULL;

  // malloc new node, check for out of memory
  if(( temp = (struct tnode*)malloc(sizeof(struct tnode))) == NULL) {
    fprintf(stderr, "tree: out of memory\n");
    return NULL;
  }

  // initilize internal pointers to NULL
  temp->parent = NULL;

  // create a new child list
  if ((temp->children = list_new ()) == NULL) {
    nodefree (temp);
    print_error ("tree: out of memory\n");
    return NULL;
  }

  return temp;
}

static void nodefree(struct tnode *node){
  list_free (node->children);
  free (node);
}

struct tnode *initialize(int upload, char pid[], char addr[]){
	
  // create new node
  struct tnode *tree = nodealloc();

  // set node variables
  tree->upload = upload;
  tree->latency = 0;
  tree->max = EU_MAX_CHILD;
  strcpy(tree->pid, pid);
  strcpy(tree->addr, addr);

  // return pointer to tree
  return tree;
}

/*
* Used to find best open spot in the tree.
* Returns a pointer to the future parent.
*/
static struct tnode *locateEmpty(struct tnode *tree){

  struct tnode *find = NULL;
  struct alpha_queue *queue;
  struct tnode *temp;

  queue = alpha_queue_new();

  if( alpha_queue_push( queue, (void*)tree) == false){
    fprintf(stderr, "tree: queue out of memory\n");
    exit(1);
  }

  // todo: in the case of no empty slots, this will loop forever
  while(find == NULL) {
    temp = (struct tnode*)alpha_queue_pop(queue);

    if(list_count (temp->children) < temp->max)
      find = temp;
    else{
      int i;
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, (void*)list_get (temp->children, i)) == false){
          fprintf(stderr, "tree: queue out of memory\n");
          exit(1);
        }
      }
    }
  }

  alpha_queue_free(queue);

  return find;

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
  new->max = EU_MAX_CHILD;
  new->parent = parent;
  strcpy(new->pid, pid);
  strcpy(new->addr, addr);

  // add to children of parent
  list_add (parent->children, new);

  // todo: SEND APPROPRIATE MESSAGES
}

static struct tnode *findPeer(struct tnode *tree, char pid[]){

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

    if(strcmp(pid, temp->pid) == 0)
      find = temp;
    else{
      int i;
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, (void*)list_get (temp->children, i)) == false){
          fprintf(stderr, "tree: queue out of memory\n");
          exit(1);
        }
      }
    }
  }while(temp != NULL && find == NULL);

  alpha_queue_free(queue);
 
  return find;
}

/*
* Used to detach a node from its parent.
*/
static void clearParent(struct tnode *remove){
  // get parent
  struct tnode *parent = remove->parent;

  list_remove_data (parent->children, remove);
}

void removePeer(struct tnode *tree, char pid[]){

  struct tnode *remove;
  int i;

  // find node to be removed in tree
  remove = findPeer(tree, pid);

  if(remove == NULL){
    fprintf(stderr, "tree: Could not find peer:\n");
    exit(1);
  }

  // move all the children
  for (i = 0; i < list_count (remove->children); i++) {
    struct tnode *child = (struct tnode*)list_get (remove->children, i);
    movePeer(tree, child->pid);
  }

  // clear this node from its parent
  clearParent(remove);

  // free node memory
  nodefree(remove);

  // todo: send messages
}

void movePeer(struct tnode *tree, char pid[]){
  
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

  // set old parents max
  oldparent->max = list_count (oldparent->children) - 1;

  // clear this node from its parent
  clearParent(move);

  // find a new spot for it and add child
  newparent = locateEmpty(tree);
  list_add (newparent->children, move);


  // todo: send messages
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
		printf("Node # of children: %d\n", list_count(print->children));
		if(print->parent != NULL)
			printf("Node Parent: %s\n", print->parent->pid);
		int i;
		for( i = 0; i < list_count(print->children); i++){
			struct tnode *child = (struct tnode*)list_get (print->children, i);
			printf("Node Child: %s\n", child->pid);
			alpha_queue_push ( queue, (void*) child);
		}
		printf("\n\n\n");

	}
}










