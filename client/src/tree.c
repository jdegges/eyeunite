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

struct tree_t *initialize(void* socket, int streambw, int peerbw, char pid[], char port[], int debug_mode)
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

  // Breadth-first search using queue, looking for parent with room for children
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

  // Breadth-first search using queue, looking for a peer in the tree
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
      alpha_queue_free(queue);
      return root;
    }
    
  }while(temp != NULL && find == NULL);

  alpha_queue_free(queue);
 
  return find;
}


/*
* Used to find a leaf node if there are no empty spots
* Returns a leaf, or NULL if none found
*/
static struct node_t *getLeaf(struct node_t *root)
{
  struct node_t *find = NULL;
  struct alpha_queue *queue;
  struct node_t *temp;
  int i;

  queue = alpha_queue_new();
  temp = root;

  // Breadth-first search using queue, looking for a leaf node (can support 0 children)
  while(find == NULL) {
    if( temp->max_c == 0)
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

/*
* This function moves a new user up the tree until their parent supports more nodes than them, or under root.
* Used in addPeer and movePeer to get peers who can support lots of others higher positions in the tree, thus
* decreasing their chances of their stream getting cut by others above them.
* Returns the node it will replace, and new node will take them as a child.
* Returns pointer to new input if it doesnt need to replace anyone.
*/
static struct node_t *switchUp(struct node_t *root, struct node_t *new, int extra)
{
  struct node_t *replace = new;

  while(replace->parent->max_c < (new->max_c - extra))
  {
    if(replace->parent != root)
      replace = replace->parent;
    else
      break;
  }

  return replace;
}

int addPeer(struct tree_t *tree, int peerbw, char pid[], char addr[], char port[])
{
  struct node_t *parent;
  struct node_t *new;
  struct node_t *replace;
  struct node_t *leaf = NULL;

  // See if the node already exists
  struct node_t *temp = findPeer(tree->root, pid);
  if (temp == NULL)
    return -1;
  if (temp != tree->root)
    return 0;

  // Try to find an empty spot on tree
  parent = locateEmpty( tree->root);
  if( parent == NULL)
    return -1;
  if(( parent == tree->root) && (tree->root->max_c == list_count( tree->root->children)))
  {
    if ( (peerbw / tree->streambw) == 0)
      return -2; // cant switch with leaf because new node cant support anyone either

    // If no empty spots, get a leaf node to switch with
    leaf = getLeaf( tree->root);
    if(leaf == NULL)
      return -1;
    if(leaf == tree->root) //no leaf found, should never happen, since we called locateEmpty already
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

  // See how far you can switch up the tree
  int extra = (leaf == NULL ? 0 : 1); // must account for leaf also
  replace = switchUp(tree->root, new, extra);

  if ( leaf != NULL)
    clearParent(leaf);

  // If its switching up, assume their children, and set them as one of your children
  if(replace != new)
  {
    clearParent(replace);
    new->parent = replace->parent;
    replace->parent = new;

    struct list *temp = new->children;
    new->children = replace->children;
    replace->children = temp;
    list_add (new->children, (void*)replace);
    int i;
    for (i = 0; i < list_count (new->children); i++)
    {
      struct node_t *child = list_get (new->children, i);
      child->parent = new;
    }
  }
  
  // Add the new node to its parents list
  list_add (new->parent->children, (void*)new);

  // If we replaced a leaf, add him as our child
  if (leaf != NULL)
  {
    list_add (new->children, (void*)leaf);
    leaf->parent = new;
  }

  // Send out appropriate messages
  if (tree->debug == 0)
  {
    sn_sendmsg ( tree->socket, new->p_info.pid, FOLLOW_NODE, &(new->parent->p_info));
    sn_sendmsg ( tree->socket, new->parent->p_info.pid, FEED_NODE, &(new->p_info));

    if ( replace != new)
      sn_sendmsg ( tree->socket, new->parent->p_info.pid, DROP_NODE, &(replace->p_info));
    
    int i;
    for (i = 0; i < list_count (new->children); i++)
    {
      struct node_t *child = list_get (new->children, i);
      sn_sendmsg ( tree->socket, child->p_info.pid, FOLLOW_NODE, &(new->p_info));
      sn_sendmsg ( tree->socket, new->p_info.pid, FEED_NODE, &(child->p_info));
      if (replace != new && child != leaf)
        sn_sendmsg (tree->socket, replace->p_info.pid, DROP_NODE, &(child->p_info));
    }

    if ( leaf != NULL)
      sn_sendmsg ( tree->socket, parent->p_info.pid, DROP_NODE, &(leaf->p_info));
  }
  return 0;
}



/*
* Helper function for removePeer. Takes tree and node, and 
* adds node to tree without worrying about its current parent.
* Similar to move peer, without oldparent
*/
static int removeAdd(struct tree_t *tree, struct node_t *add)
{
  struct node_t *newparent;
  struct node_t *leaf = NULL;
  struct node_t *replace;

  // Find a new empty spot in tree
  newparent = locateEmpty(tree->root);
  if (newparent == NULL)
    return -1;
  if ((newparent == tree->root) && (tree->root->max_c == list_count( tree->root->children))) // If no empty spots are found
  {
    if (add->max_c == 0 || list_count (add->children) == add->max_c) //might want to drop leaf if he has full children list
      return -3;

    // Look for a leaf if no empty spots
    leaf = getLeaf (tree->root);
    if (leaf == NULL)
      return -1;
    if (leaf == tree->root) //no leaf found, should never happen, since we called locateEmpty already
      return -3;

    newparent = leaf->parent;
  }

  add->parent = newparent;

  // See how far we can switch up in the new spot
  int extra = list_count(add->children) + (leaf == NULL ? 0 : 1); // must account for current # chlidren and possible leaf
  replace = switchUp(tree->root, add, extra);

  if ( leaf != NULL)
    clearParent(leaf);

  // If replacing someone, take all their children and also feed the one you're replacing
  if(replace != add)
  {
    clearParent(replace);
    add->parent = replace->parent;
    replace->parent = add;

    list_add (add->children, (void*)replace);
    while (list_count (replace->children) > 0)
    {
      struct node_t *child = (struct node_t*)list_get (replace->children, 0);
      list_add (add->children, (void*)child);
      list_remove_item (replace->children, 0);
      child->parent = add;
    }

  }

  // Add moved node to new parent
  list_add (add->parent->children, (void*)add);

  // If we took leaf's spot, add him to our children
  if (leaf != NULL)
  {
    list_add (add->children, (void*)leaf);
    leaf->parent = add;
  }

  // Send appropriate messages
  if (tree->debug == 0) {
    sn_sendmsg ( tree->socket, add->p_info.pid, FOLLOW_NODE, &(add->parent->p_info));
    sn_sendmsg ( tree->socket, add->parent->p_info.pid, FEED_NODE, &(add->p_info));

    if ( replace != add)
      sn_sendmsg ( tree->socket, add->parent->p_info.pid, DROP_NODE, &(replace->p_info));
    
    int i;
    for (i = 0; i < list_count (add->children); i++)
    {
      struct node_t *child = list_get (add->children, i);
      sn_sendmsg ( tree->socket, child->p_info.pid, FOLLOW_NODE, &(add->p_info));
      sn_sendmsg ( tree->socket, add->p_info.pid, FEED_NODE, &(child->p_info));
      if (replace != add && child != leaf)
        sn_sendmsg (tree->socket, replace->p_info.pid, DROP_NODE, &(child->p_info));
    }

    if ( leaf != NULL)
      sn_sendmsg ( tree->socket, newparent->p_info.pid, DROP_NODE, &(leaf->p_info));
  }
}

int removePeer(struct tree_t *tree, char pid[])
{
  struct node_t *remove;
  struct node_t *max;
  struct node_t *child;
  int i;

  // Find the peer to be removed
  remove = findPeer(tree->root, pid);

  if (remove == NULL)
    return -1;
  if (remove == tree->root) // Peer not found, return success
    return 0;

  clearParent(remove);

  for (i = 0; i < list_count (remove->children); i++)
  {
    struct node_t *child = list_get (remove->children, i);
    removeAdd(tree, child);
  }

  // find a peer in subtree to take my spot.. (most open spots)
  // if enough open spots, give all children to subtree find
  // if not, move enough children to fit the rest in subtree find
  
  if (tree->debug == 0) {
    sn_sendmsg( tree->socket, remove->parent->p_info.pid, DROP_NODE, &(remove->p_info));
  }

  nodefree(remove);

  // todo: maybe tell remove to stop listening for parent
  return 0;
}


int movePeer(struct tree_t *tree, char pid[])
{
  struct node_t *newparent;
  struct node_t *move;
  struct node_t *oldparent;
  struct node_t *leaf = NULL;
  struct node_t *replace;

  // Find the pair to be moved
  move = findPeer(tree->root, pid);

  if( move == NULL)
    return -1;
  if( move == tree->root) // If not in tree, return error code -2
    return -2;

  // Change parent's max children who was sending bad feed
  oldparent = move->parent;
  oldparent->max_c = list_count (oldparent->children) - 1;
  clearParent(move);

  // Find a new empty spot in tree
  newparent = locateEmpty(tree->root);
  if (newparent == NULL)
    return -1;
  if ((newparent == tree->root) && (tree->root->max_c == list_count( tree->root->children))) // If no empty spots are found
  {
    if (move->max_c == 0 || list_count (move->children) == move->max_c) //might want to drop leaf if he has full children list
      return -3;

    // Look for a leaf if no empty spots
    leaf = getLeaf (tree->root);
    if (leaf == NULL)
      return -1;
    if (leaf == tree->root) //no leaf found, should never happen, since we called locateEmpty already
      return -3;

    newparent = leaf->parent;
  }

  move->parent = newparent;

  // See how far we can switch up in the new spot
  int extra = list_count(move->children) + (leaf == NULL ? 0 : 1); // must account for current # chlidren and possible leaf
  replace = switchUp(tree->root, move, extra);

  if ( leaf != NULL)
    clearParent(leaf);

  // If replacing someone, take all their children and also feed the one you're replacing
  if(replace != move)
  {
    clearParent(replace);
    move->parent = replace->parent;
    replace->parent = move;

    list_add (move->children, (void*)replace);
    while (list_count (replace->children) > 0)
    {
      struct node_t *child = (struct node_t*)list_get (replace->children, 0);
      list_add (move->children, (void*)child);
      list_remove_item (replace->children, 0);
      child->parent = move;
    }

  }

  // Add moved node to new parent
  list_add (move->parent->children, (void*)move);

  // If we took leaf's spot, add him to our children
  if (leaf != NULL)
  {
    list_add (move->children, (void*)leaf);
    leaf->parent = move;
  }

  // Send appropriate messages
  if (tree->debug == 0) {
    sn_sendmsg ( tree->socket, move->p_info.pid, FOLLOW_NODE, &(move->parent->p_info));
    sn_sendmsg ( tree->socket, move->parent->p_info.pid, FEED_NODE, &(move->p_info));
    sn_sendmsg ( tree->socket, oldparent->p_info.pid, DROP_NODE, &(move->p_info));

    if ( replace != move)
      sn_sendmsg ( tree->socket, move->parent->p_info.pid, DROP_NODE, &(replace->p_info));
    
    int i;
    for (i = 0; i < list_count (move->children); i++)
    {
      struct node_t *child = list_get (move->children, i);
      sn_sendmsg ( tree->socket, child->p_info.pid, FOLLOW_NODE, &(move->p_info));
      sn_sendmsg ( tree->socket, move->p_info.pid, FEED_NODE, &(child->p_info));
      if (replace != move && child != leaf)
        sn_sendmsg (tree->socket, replace->p_info.pid, DROP_NODE, &(child->p_info));
    }

    if ( leaf != NULL)
      sn_sendmsg ( tree->socket, newparent->p_info.pid, DROP_NODE, &(leaf->p_info));
  }
}

/*
* Used to free tree starting from root
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
	
	queue = alpha_queue_new();
	
	alpha_queue_push( queue, (void*) tree->root);
  fprintf(stdout, "\n\n");

	while(( print = (struct node_t*)alpha_queue_pop(queue)) != NULL) {
		fprintf(stdout, "Node PID: %s\n", print->p_info.pid);
		fprintf(stdout, "Node # of children: %llu\n", list_count(print->children));
    fprintf(stdout, "Node max children: %d\n", print->max_c);
		if(print->parent != NULL)
			fprintf(stdout, "Node Parent: %s\n", print->parent->p_info.pid);
		int i;
		for( i = 0; i < list_count(print->children); i++){
			struct node_t *child = (struct node_t*)list_get (print->children, i);
			fprintf(stdout, "Node Child: %s\n", child->p_info.pid);
			alpha_queue_push ( queue, (void*) child);
		}
		fprintf(stdout, "\n\n");
	}
}


void* getSocket(struct tree_t *tree)
{
  return tree->socket;
}

uint64_t countRootChildren(struct tree_t *tree)
{
  return list_count (tree->root->children);
}

struct peer_info *getRootChild(struct tree_t *tree, uint64_t pos)
{
  struct node_t *child = list_get (tree->root->children, pos);
  return &child->p_info;
}
