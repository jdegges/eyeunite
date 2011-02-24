#include <stdio.h>
#include <stdlib.h>

#include "sourcenode.h"
#include "tree.h"

int main(void) {

  // Create a port
  // Substitute bootsrap in here later
  const char* pid = "111";
  const char* endpoint = "tcp://*:55555";
  void* sock = sn_initzmq (endpoint, pid);

  // Create tree -- needs bootstrap
  struct tree_t* tree = initialize (sock, 5, 100, "111", "127.0.0.1", 55555, 0);

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
                printf ("Memory Error in moving peer\n");
                break;
              case -2:
                printf ("Peer Not Found in Tree in moving peer\n");
                break;
              case -3:
                printf ("No Empty Slots in moving peer\n");
                break;
            }
          }
          break;

        case REQ_JOIN:
          ;int pradd = addPeer (tree, msg->node_params.peerbw, msg->node_params.pid, msg->node_params.addr, msg->node_params.port);
          if (pradd != 0) {
              switch (pradd) {
                case -1:
                  printf ("Memory Error in adding peer\n");
                  break;
                case -2:
                  printf ("No Empty Slots in adding peer\n");
                  break;
              }
          }
          break;

        case REQ_EXIT:
          ;int prrmv = removePeer (tree, msg->node_params.pid);
          if (prrmv == -1) {
            printf ("Memory Error in remove peer\n");
          }
        }
      }
    }

sn_closesocket (sock);
freetree (tree);
    
return;
}
