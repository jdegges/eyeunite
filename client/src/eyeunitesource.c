#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "sourcenode.h"
#include "tree.h"
#include "debug.h"
#include "eyeunite.h"
#include "bootstrap.h"


int main(void) {

  // Create a port
  // Substitute bootsrap in here later
  char pid[EU_TOKENSTRLEN];
  char lt[EU_TOKENSTRLEN];
  struct bootstrap* btstr = bootstrap_init (APP_ENGINE, 55555, pid, NULL);
  if (bootstrap_lobby_create (btstr, lt)) {
    print_error ("Couldn't create a lobby\n");
  }
  const char* endpoint = "tcp://*:55555";
  void* sock = sn_initzmq (endpoint, pid);

  // Create tree -- needs bootstrap
  struct tree_t* tree = initialize (sock, 5, 100, pid, "127.0.0.1", 55555, 0);

  printTree (tree);

  while (1) {
    message_struct* msg = NULL;
    msg = sn_rcvmsg(sock);

    if (msg != NULL) {
      switch (msg->type) {
        case REQ_MOVE:
          ;int prmve = movePeer(tree, msg->node_params.pid);
          if (prmve != 0) {
            switch (prmve) {
              case -1:
                print_error ("Memory Error in moving peer\n");
                break;
              case -2:
                print_error ("Peer Not Found in Tree in moving peer\n");
                break;
              case -3:
                print_error ("No Empty Slots in moving peer\n");
                break;
            }
          }
          //printTree (tree);
          break;

        case REQ_JOIN:
          ;int pradd = addPeer (tree, msg->node_params.peerbw, msg->node_params.pid, msg->node_params.addr, msg->node_params.port);
          if (pradd != 0) {
              switch (pradd) {
                case -1:
                  print_error ("Memory Error in adding peer\n");
                  break;
                case -2:
                  print_error ("No Empty Slots in adding peer\n");
                  break;
              }
          }
          //printTree (tree);
          break;

        case REQ_EXIT:
          ;int prrmv = removePeer (tree, msg->node_params.pid);
          if (prrmv == -1) {
            print_error ("Memory Error in remove peer\n");
          }
          //printTree (tree);
      }
    }
  }

  if (bootstrap_lobby_leave (btstr)) {
    print_error ("Error leaving lobby\n");
  }
  bootstrap_cleanup (btstr);
  sn_closesocket (sock);
  freetree (tree);
    
  return;
}
