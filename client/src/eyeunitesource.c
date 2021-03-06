#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "alpha_queue.h"
#include "bootstrap.h"
#include "debug.h"
#include "easyudp.h"
#include "eyeunite.h"
#include "sourcenode.h"
#include "tree.h"

static void* control_thread(void *vptr) {
  struct tree_t* tree = vptr;
  void* sock = getSocket(tree);

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
          ;int pradd = addPeer (tree, msg->node_params.peerbw,
                                msg->node_params.pid, msg->node_params.addr,
                                msg->node_params.port);
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
        
        case REM_NODE:
          print_error ("Request drop node %s\n", msg->node_params.pid);
          ;int prremove = removePeer (tree, msg->node_params.pid);
          
          if (pradd != 0) {
              switch (pradd) {
                case -1:
                  print_error ("Memory Error in adding peer\n");
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

  return NULL;
}

char *media_file;

static void* data_thread(void *vptr) {
  struct tree_t* tree = vptr;
  struct data_pack dpack;
  FILE* fp = fopen (media_file, "rb");
  struct alpha_queue *socks;
  socks = alpha_queue_new();

  if (NULL == fp) {
    print_error ("error opening media file");
    return NULL;
  }

  dpack.seqnum = 1;
  for (;;) {
    size_t amount = fread (dpack.data, 1, EU_PACKETLEN - sizeof (uint64_t), fp);
    if (0 == amount) {
      print_error ("end of file..?");
      return NULL;
    }

    while (0 == countRootChildren(tree)) {
      sleep(1);		// Temporary to allow setup for tests
      // do nothing.
    }

    uint64_t i;
    for (i = 0; i < countRootChildren(tree); i++) {
      struct peer_info *pi = getRootChild (tree, i);
      if (NULL == pi)
        break;

      struct eu_socket *sock = alpha_queue_pop (socks);
      if (NULL == sock) {
        sock = eu_socket (EU_PUSH);
        if (NULL == sock) {
          print_error ("no more easy socks?");
          return NULL;
        }
      }

      if (eu_connect (sock, pi->addr, pi->port)) {
        print_error ("couldn't connect??");
        return NULL;
      }
      
      int rc = eu_send (sock, &dpack, amount + sizeof (uint64_t), 0);
      if (rc != (amount + sizeof (uint64_t))) {
        if (rc == -2)
        {
          int prremove = removePeer (tree, pi->pid);
          
          if (prremove != 0) {
              switch (prremove) {
                case -1:
                  print_error ("Memory Error in removing peer\n");
                  break;
              }
          }
          printTree (tree);
        }
        
        print_error ("couldn't send full packet :/");
        return NULL;
      }
      
      alpha_queue_push (socks, sock);
      
      usleep(200);
    }

    dpack.seqnum++;
  }
}

int main(int argc, char* argv[]) {

  if (argc != 4) {
    printf ("Usage:\n");
    printf ("./eyeunitesource <listenport> <bandwidth> <media file>\n");
    return;
  }

  // Create a port
  // Substitute bootsrap in here later
  char pid[EU_TOKENSTRLEN];
  char lt[EU_TOKENSTRLEN];
  char port[EU_PORTSTRLEN];
  memcpy (port, argv[1], EU_PORTSTRLEN);
  int bw = atoi (argv[2]);
  media_file = argv[3]; // global
  
  struct bootstrap* btstr = bootstrap_init (APP_ENGINE, port, pid, NULL);
  if (bootstrap_lobby_create (btstr, lt)) {
    print_error ("Couldn't create a lobby\n");
  }
  char endpoint[EU_ADDRSTRLEN*4];
  snprintf(endpoint, EU_ADDRSTRLEN*4, "tcp://*:%s", port);
  void* sock = sn_initzmq (endpoint, pid);

  // Create tree -- needs bootstrap
  struct tree_t* tree = initialize (sock, 5, bw, pid, port, 0);

  printTree (tree);

  pthread_t control_thread_id;
  pthread_create (&control_thread_id, NULL, control_thread, tree);

  pthread_t data_thread_id;
  pthread_create (&data_thread_id, NULL, data_thread, tree);

  pthread_join (data_thread_id, NULL);
  pthread_join (control_thread_id, NULL);

  if (bootstrap_lobby_leave (btstr)) {
    print_error ("Error leaving lobby\n");
  }
  bootstrap_cleanup (btstr);
  sn_closesocket (sock);
  freeTree (tree);
    
  return;
}
