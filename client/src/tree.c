#include "eyeunite.h"
#include "tree.h"
#include "alpha_queue.h"
//#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct tree_t
{
  struct node_t *root;
  int streambw;
};


struct node_t
{
  int max_c;
  struct peer_info p_info;
  struct list *children;
  struct node_t *parent;
};

static void nodefree(struct node_t *node)
{
  list_free (node->children);
  free (node);
}

/*
* Used to allocate a new node.
* Returns a pointer to the new node.
*/
static struct node_t *nodealloc(void)
{
  struct node_t *temp = NULL;

  // malloc new node, check for out of memory
  if(( temp = (struct node_t*)malloc( sizeof( struct node_t))) == NULL) {
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

struct tree_t *initialize(int streambw, int peerbw, char pid[], char addr[], uint16_t port)
{
  // create new tree and node
  struct tree_t *tree;

  if(( tree = malloc( sizeof( struct tree_t))) == NULL){
    fprintf(stderr, "tree: out of memory\n");
    return NULL;
  }

  if(( tree->root = nodealloc()) == NULL){
    freeTree(tree);
    return NULL;
  }

  // set node variables
  tree->streambw = streambw;
  tree->root->p_info.peerbw = peerbw;
  tree->root->p_info.port = port;
  tree->root->max_c = peerbw / streambw;
  strcpy(tree->root->p_info.pid, pid);
  strcpy(tree->root->p_info.addr, addr);

  // return pointer to tree
  return tree;
}

/*
* Used to find best open spot in the tree.
* Returns a pointer to the future parent.
*/
static struct node_t *locateEmpty(struct node_t *root)
{
  struct node_t *find = NULL;
  struct alpha_queue *queue;
  struct node_t *temp;

  queue = alpha_queue_new();

  if( alpha_queue_push( queue, (void*)root) == false){
    fprintf(stderr, "tree: queue out of memory\n");
    alpha_queue_free(queue);
    return NULL;
  }

  while(find == NULL) {
    if( (temp = (struct node_t*)alpha_queue_pop( queue)) == NULL){
      fprintf(stderr, "tree: no empty slots\n");
      alpha_queue_free(queue);
      return NULL;
    }

    if( list_count (temp->children) < temp->max_c)
      find = temp;
    else{
      int i;
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, (void*)list_get (temp->children, i)) == false){
          fprintf(stderr, "tree: queue out of memory\n");
          alpha_queue_free(queue);
          return NULL;
        }
      }
    }
  }

  alpha_queue_free(queue);

  return find;

}

void addPeer(struct tree_t *tree, int peerbw, char pid[], char addr[], uint16_t port)
{
  struct node_t *parent;
  struct node_t *new;

  // locate a parent for this peer
  parent = locateEmpty( tree->root)
  if( parent == NULL)
    exit(1); //todo: change to error report

  // create a new node
  new = nodealloc()
  if( new == NULL)
    exit(1); //todo: change to error report

  // set node variables
  new->p_info.peerbw = peerbw;
  new->p_info.port = port;
  new->max_c = peerbw / tree->streambw;
  new->parent = parent;
  strcpy(new->p_info.pid, pid);
  strcpy(new->p_info.addr, addr);

  // add to children of parent
  list_add (parent->children, new);

  // todo: SEND APPROPRIATE MESSAGES
}

static struct node_t *findPeer(struct node_t *root, char pid[])
{
  struct node_t *find = NULL;
  struct alpha_queue *queue;
  struct node_t *temp;
  int i;

  queue = alpha_queue_new();

  if( alpha_queue_push( queue, (void*)root) == false){
    fprintf(stderr, "tree: queue out of memory\n");
    alpha_queue_free(queue);
    return NULL;
  }

  do{
    if(( temp = (struct node_t*)alpha_queue_pop(queue)) == NULL){
      fprintf(stderr, "tree: could not find peer\n");
      alpha_queue_free(queue);
      return NULL;
    }

    if(strcmp(pid, temp->p_info.pid) == 0)
      find = temp;
    else{
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, (void*)list_get (temp->children, i)) == false){
          fprintf(stderr, "tree: queue out of memory\n");
          alpha_queue_free(queue);
          return NULL;
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
static void clearParent(struct node_t *remove)
{
  // get parent
  struct node_t *parent = remove->parent;

  list_remove_data (parent->children, remove);
}

void removePeer(struct tree_t *tree, char pid[])
{
  struct node_t *remove;
  int i;

  // find node to be removed in tree
  remove = findPeer(tree->root, pid);

  if(remove == NULL)
    exit(1); //todo: report not in tree

  // move all the children
  for (i = 0; i < list_count (remove->children); i++) {
    struct node_t *child = (struct node_t*)list_get (remove->children, i);
    movePeer(tree, child->p_info.pid);
  }

  // clear this node from its parent
  clearParent(remove);

  // free node memory
  nodefree(remove);

  // todo: send messages
}

void movePeer(struct tree_t *tree, char pid[])
{
  struct node_t *newparent;
  struct node_t *move;
  struct node_t *oldparent;

  // find node to be moved
  move = findPeer(tree->root, pid);

  if( move == NULL)
    exit(1); // todo: report node not in tree

  // save old parent pointer and set their max
  oldparent = move->parent;
  oldparent->max_c = list_count (oldparent->children) - 1;

  // clear this node from its parent
  clearParent(move);

  // find a new spot for it and add child
  newparent = locateEmpty(tree->root)
  if( newparent == NULL)
    exit(1); //todo: report error 

  list_add (newparent->children, move);


  // todo: send messages
}

void freeTree(struct tree_t *tree)
{
  struct alpha_queue *queue;

  queue = alpha_queue_new();
  
  if( alpha_queue_push( queue, (void*)tree->root) == false){
  }
}


//debug functions
void printTree(struct tree_t *tree)
{
	// print tree recursively (depth first search)
	struct alpha_queue *queue;
	struct node_t *print;
	
	queue = alpha_queue_new();
	
	alpha_queue_push ( queue, (void*) tree->root);

	while(( print = (struct node_t*)alpha_queue_pop(queue)) != NULL) {
		printf("Node PID: %s\n", print->p_info.pid);
		printf("Node IP: %s\n", print->p_info.addr);
		printf("Node # of children: %d\n", list_count(print->children));
		if(print->parent != NULL)
			printf("Node Parent: %s\n", print->parent->p_info.pid);
		int i;
		for( i = 0; i < list_count(print->children); i++){
			struct node_t *child = (struct node_t*)list_get (print->children, i);
			printf("Node Child: %s\n", child->p_info.pid);
			alpha_queue_push ( queue, (void*) child);
		}
		printf("\n\n\n");

	}
}










