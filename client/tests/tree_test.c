#include "tree.h"
#include "eyeunite.h"

#include <assert.h>

void addTestSimple();
void addTestSwitch();
void addTestLeaf();
void addTestSwitchandLeaf();
void addTestNoSpots();

int main (void){

  //addPeer tests
	addTestSimple();
  addTestSwitch();
  addTestLeaf();
  addTestSwitchandLeaf();
  addTestNoSpots();
}	

//Simple add test
void addTestSimple(){
  struct tree_t* simpleadd = initialize(NULL, 1, 2, "addP", "ip:root", "p:root", 0);
  
	addPeer (simpleadd, 2, "add:1", "ip:1", "port:1");
	addPeer (simpleadd, 2, "add:2", "ip:2", "port:2");
  addPeer (simpleadd, 2, "add:3", "ip:3", "port:3");
  addPeer (simpleadd, 2, "add:4", "ip:4", "port:4");
  addPeer (simpleadd, 2, "add:5", "ip:5", "port:5");
  
  printTree(simpleadd);
}

//Tests that new peer switches up correctly
void addTestSwitch(){
  struct tree_t* switchup = initialize(NULL, 1, 2, "addup", "ip:root", "p:root", 0);
  
	addPeer (switchup, 2, "add:1", "ip:1", "port:1");
	addPeer (switchup, 1, "add:2", "ip:2", "port:2");
  addPeer (switchup, 2, "add:3", "ip:3", "port:3");
  addPeer (switchup, 4, "add:4", "ip:4", "port:4");
  addPeer (switchup, 3, "add:5", "ip", "port");
  addPeer (switchup, 6, "add:6", "ip", "port");

  printTree(switchup);
}

//Tests that new peer takes spot of leaf
void addTestLeaf(){
  struct tree_t *leaf = initialize(NULL, 1, 2, "nospot", "ip", "port", 0);

  addPeer (leaf, 0, "add:1", "ip", "port");
  addPeer (leaf, 0, "add:2", "ip", "port");

  addPeer (leaf, 1, "add:3", "ip", "port");

  printTree(leaf);
}

//Tests that new peer takes spot of leaf and switches up
void addTestSwitchandLeaf(){
  struct tree_t *switchleaf = initialize(NULL, 1, 2, "switch", "ip", "port", 0);

  addPeer (switchleaf, 2, "add:1", "ip", "port");
  addPeer (switchleaf, 1, "add:2", "ip", "port");
  addPeer (switchleaf, 0, "add:3", "ip", "port");
  addPeer (switchleaf, 0, "add:4", "ip", "port");
  addPeer (switchleaf, 0, "add:5", "ip", "port");
  addPeer (switchleaf, 1, "add:6", "ip", "port");
  addPeer (switchleaf, 4, "add:7", "ip", "port");

  printTree(switchleaf);
}

//Tests that addPeer returns true when a new leaf tries to join with no spots
void addTestNoSpots(){
  struct tree_t *nospots = initialize(NULL, 1, 2, "nospot", "ip", "port", 0);

  addPeer (nospots, 0, "add:1", "ip", "port");
  addPeer (nospots, 0, "add:2", "ip", "port");

  assert( addPeer(nospots, 0, "add:3", "ip", "port") == -2);
}





