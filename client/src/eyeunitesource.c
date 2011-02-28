#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "sourcenode.h"
#include "tree.h"
#include "debug.h"
#include "eyeunite.h"
#include "bootstrap.h"


int main(int argc, char* argv[]) {

  if (argc != 4) {
    printf ("Usage:\n");
    printf ("./eyeunitesource <ipaddress> <listenport> <bandwidth>\n");
    return;
  }

  // Create a port
  // Substitute bootsrap in here later
  char pid[EU_TOKENSTRLEN];
  char lt[EU_TOKENSTRLEN];
  char ipadd[EU_ADDRSTRLEN];
  memcpy (ipadd, argv[1], EU_ADDRSTRLEN);
  int port = atoi (argv[2]);
  int bw = atoi (argv[3]);
  
  struct bootstrap* btstr = bootstrap_init (APP_ENGINE, port, pid, NULL);
  if (bootstrap_lobby_create (btstr, lt)) {
    print_error ("Couldn't create a lobby\n");
  }
  const char endpoint[EU_ADDRSTRLEN*4];
  snprintf(endpoint, EU_ADDRSTRLEN*4, "tcp://*:%d", port);
  void* sock = sn_initzmq (endpoint, pid);

  // Create tree -- needs bootstrap
  struct tree_t* tree = initialize (sock, 5, bw, pid, ipadd, port, 0);

  printTree (tree);

  while (1) {
    message_struct* msg = NULL;
    msg = sn_rcvmsg(sock);
    
    if (msg != NULL) {

      switch (msg->type) {
        case REQ_MOVE:
          print_error ("Request move from %s\n", msg->node_params.pid);
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
          printTree (tree);
          break;

        case REQ_JOIN:
          print_error ("Request join from %s\n", msg->node_params.pid);
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
          printTree (tree);
          break;

        case REQ_EXIT:
          print_error ("Request exit from %s\n", msg->node_params.pid);
          ;int prrmv = removePeer (tree, msg->node_params.pid);
          if (prrmv == -1) {
            print_error ("Memory Error in remove peer\n");
          }
          printTree (tree);
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
