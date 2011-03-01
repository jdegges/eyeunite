#include "eyeunite.h"
#include "tree.h"
#include "alpha_queue.h"
#include "list.h"
#include "debug.h"
#include "sourcenode.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct tree_t{
  struct node_t *root;
  int streambw;
  void* socket;
  bool debug;
};


struct node_t{
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
* Returns a pointer to the new node or NULL if memory error.
*/
static struct node_t *nodealloc(void)
{
  struct node_t *temp;

  if(( temp = (struct node_t*)malloc( sizeof( struct node_t))) == NULL) {
    print_error ("out of memory");
    return NULL;
  }


  temp->parent = NULL;
  if ((temp->children = list_new ()) == NULL) {
    nodefree (temp);
    print_error ("out of memory");
    return NULL;
  }

  return temp;
}

struct tree_t *initialize(void* socket, int streambw, int peerbw, char pid[], char addr[], char port[], int debug_mode)
{
  struct tree_t *tree;

  if(( tree = (struct tree_t*)malloc( sizeof( struct tree_t))) == NULL){
    print_error ("out of memory");
    return NULL;
  }

  if(( tree->root = nodealloc()) == NULL){
    freeTree(tree);
    return NULL;
  }

  tree->socket = socket;
  tree->streambw = streambw;
  tree->debug = debug_mode;
  tree->root->p_info.peerbw = peerbw;
  tree->root->max_c = peerbw / streambw;
  strcpy(tree->root->p_info.pid, pid);
  strcpy(tree->root->p_info.addr, addr);
  strcpy(tree->root->p_info.port, port);

  return tree;
}

/*
* Used to find best open spot in the tree.
* Returns a pointer to the future parent if empty spot is found.
* Otherwise returns NULL if memory error or root if no empty spots found.
* For no empty spots, must check that root doesnt have empty spot afterwards.
*/
static struct node_t *locateEmpty(struct node_t *root)
{
  struct node_t *find = NULL;
  struct alpha_queue *queue;
  struct node_t *temp;
  int i;

  queue = alpha_queue_new();
  temp = root;

  while(find == NULL) {
    if( list_count (temp->children) < temp->max_c)
      find = temp;
    else{
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, list_get (temp->children, i)) == false){
          print_error( "out of memory");
          alpha_queue_free(queue);
          return NULL;
        }
      }
      if(( temp = (struct node_t*)alpha_queue_pop( queue)) == NULL){
        print_error( "no empty slots on tree");
        alpha_queue_free(queue);
        return root;
      }
    }  
  }

  alpha_queue_free(queue);

  return find;

}

/*
* Used to detach a node from its parent.
*/
static void clearParent(struct node_t *remove)
{
  struct node_t *parent = remove->parent;

  list_remove_data (parent->children, (void*)remove);
}

/*
* Used to find a leaf node if there are no empty spots
*/
static struct node_t *getLeaf(struct node_t *root)
{
  struct node_t *find = NULL;
  struct alpha_queue *queue;
  struct node_t *temp;
  int i;

  queue = alpha_queue_new();
  temp = root;

  while(find == NULL) {
    if( temp != root && temp->max_c == 0)
      find = temp;
    else{
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, list_get (temp->children, i)) == false){
          print_error( "out of memory");
          alpha_queue_free(queue);
          return NULL;
        }
      }
      if(( temp = (struct node_t*)alpha_queue_pop( queue)) == NULL){
        print_error( "catastrophic: no leaf found");
        alpha_queue_free(queue);
        return root;
      }
    }  
  }

  alpha_queue_free(queue);

  return find;
}

int addPeer(struct tree_t *tree, int peerbw, char pid[], char addr[], char port[])
{
  struct node_t *parent;
  struct node_t *new;
  struct node_t *leaf = NULL;

  parent = locateEmpty( tree->root); //might want to optimize when cant find empty slot, unless still no empty spot in optimization
  if( parent == NULL)
    return -1;
  if(( parent == tree->root) && (tree->root->max_c == list_count( tree->root->children)))
  {
    if ( (peerbw / tree->streambw) == 0)
      return -2; //out of luck, cant switch cus old leaf will have to drop

    leaf = getLeaf( tree->root);
    if(leaf == NULL)
      return -1;
    if(leaf == tree->root) //no leaf found, should never happen
      return -2;
    parent = leaf->parent;
  }

  new = nodealloc();
  if( new == NULL)
    return -1;

  new->p_info.peerbw = peerbw;
  new->max_c = peerbw / tree->streambw;
  new->parent = parent;
  strcpy(new->p_info.pid, pid);
  strcpy(new->p_info.addr, addr);
  strcpy(new->p_info.port, port);

  if (leaf != NULL)
  {
    list_add (new->children, (void*)leaf);
    clearParent(leaf);
    leaf->parent = new;
  }

  list_add (parent->children, (void*)new);

  if (tree->debug == 0) //maybe switch around order to avoid a slowdown if parent cant feed both (might initiate a peer move request)
  {
    sn_sendmsg ( tree->socket, new->p_info.pid, FOLLOW_NODE, &(parent->p_info));
    sn_sendmsg ( tree->socket, parent->p_info.pid, FEED_NODE, &(new->p_info));
    if ( leaf != NULL)
    {
      sn_sendmsg ( tree->socket, leaf->p_info.pid, FOLLOW_NODE, &(new->p_info));
      sn_sendmsg ( tree->socket, new->p_info.pid, FEED_NODE, &(leaf->p_info));
      sn_sendmsg ( tree->socket, parent->p_info.pid, DROP_NODE, &(leaf->p_info));
    }
  }
  return 0;
}


/*
* Used to find a peer in the tree.
* Returns pointer to peer if found.
* Returns NULL if memory error.
* Returns root if node not found.
*/
static struct node_t *findPeer(struct node_t *root, char pid[])
{
  struct node_t *find = NULL;
  struct alpha_queue *queue;
  struct node_t *temp;
  int i;

  queue = alpha_queue_new();
  temp = root;

  do{
    if(strcmp(pid, temp->p_info.pid) == 0)
      find = temp;
    else{
      for ( i = 0; i < list_count (temp->children); i++){
        if( alpha_queue_push(queue, list_get (temp->children, i)) == false){
          print_error( "out of memory");
          alpha_queue_free(queue);
          return NULL;
        }
      }
    }

    if(( temp = (struct node_t*)alpha_queue_pop(queue)) == NULL && find == NULL){
      fprintf(stderr, "tree: could not find peer\n");
      alpha_queue_free(queue);
      return root;
    }
    
  }while(temp != NULL && find == NULL);

  alpha_queue_free(queue);
 
  return find;
}



int removePeer(struct tree_t *tree, char pid[])
{
  struct node_t *remove;
  struct node_t *child;
  int i;

  remove = findPeer(tree->root, pid);

  if(remove == NULL)
    return -1;
  if(remove == tree->root)
    return 0;

  // find a peer in subtree to take my spot.. (most open spots)
  // if enough open spots, give all children to subtree find
  // if not, move enough children to fit the rest in subtree find
  while ( list_count (remove->children) > 0){
    child = (struct node_t*)list_get( remove->children, 0);
    movePeer(tree, child->p_info.pid);
  }

  clearParent(remove);
  nodefree(remove);

  if (tree->debug == 0) {
    sn_sendmsg( tree->socket, remove->parent->p_info.pid, DROP_NODE, &(remove->p_info));
  }
  // todo: maybe tell remove to stop listening for parent
  return 0;
}

int movePeer(struct tree_t *tree, char pid[])
{
  struct node_t *newparent;
  struct node_t *move;
  struct node_t *oldparent;

  move = findPeer(tree->root, pid);

  if( move == NULL)
    return -1;
  if( move == tree->root)
    return -2;

  oldparent = move->parent;
  oldparent->max_c = list_count (oldparent->children) - 1;

  clearParent(move);

  newparent = locateEmpty(tree->root); //if no empty. should switch with a leaf again (pending this one can handle more children) (on 2nd if below, like addPeer)
  if( newparent == NULL)
    return -1;
  if(( newparent == tree->root) && (tree->root->max_c == list_count( tree->root->children)))
    return -3;

  list_add (newparent->children, (void*)move);
  move->parent = newparent;

  if (tree->debug == 0) {
    sn_sendmsg ( tree->socket, newparent->p_info.pid, FEED_NODE, &(move->p_info));
    sn_sendmsg ( tree->socket, move->p_info.pid, FOLLOW_NODE, &(newparent->p_info));
    sn_sendmsg ( tree->socket, oldparent->p_info.pid, DROP_NODE, &(move->p_info));
  }
  // todo: maybe send to move to drop oldparent
}

/*
* Used to free tree from root
*/
static void freeRoot(struct node_t *root)
{
  int i;

  if( root == NULL)
    return;
  for( i = 0; i < list_count(root->children); i++){
    freeRoot( (struct node_t*)list_get( root->children, i));
  }
  free(root);
}

/*
 * Used to free entire tree structure
 */ 
void freeTree(struct tree_t *tree)
{
  if( tree == NULL)
    return;

  freeRoot( tree->root);
  free( tree);  
}


//debug functions
void printTree(struct tree_t *tree)
{
	struct alpha_queue *queue;
	struct node_t *print;
	FILE *f;
	
	f = fopen (FILELOC, "a+");
	
	
	queue = alpha_queue_new();
	
	alpha_queue_push( queue, (void*) tree->root);
fprintf(f, "%s\n", "test");

	while(( print = (struct node_t*)alpha_queue_pop(queue)) != NULL) {
		fprintf(f, "Node PID: %s\n", print->p_info.pid);
		fprintf(f, "Node IP: %s\n", print->p_info.addr);
		fprintf(f, "Node # of children: %llu\n", list_count(print->children));
    fprintf(f, "Node max children: %d\n", print->max_c);
		if(print->parent != NULL)
			fprintf(f, "Node Parent: %s\n", print->parent->p_info.pid);
		int i;
		for( i = 0; i < list_count(print->children); i++){
			struct node_t *child = (struct node_t*)list_get (print->children, i);
			fprintf(f, "Node Child: %s\n", child->p_info.pid);
			alpha_queue_push ( queue, (void*) child);
		}
		fprintf(f, "BREAK!!!!!!\n\n\n");

	}
	fclose(f);
}










