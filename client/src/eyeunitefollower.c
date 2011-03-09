#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#include "bootstrap.h"
#include "followernode.h"
#include "tree.h"
#include "debug.h"
#include "easyudp.h"
#include "alpha_queue.h"
#include "time.h"

// Global variables for threads
struct peer_info source_peer;
void* source_zmq_sock = NULL;
struct eu_socket* upstream_eu_sock = NULL;
struct peer_node* downstream_peers = NULL;
size_t num_downstream_peers = 0;
pthread_mutex_t downstream_peers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t source_peer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t packet_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
struct peer_info my_peer_info;
int my_bw = 1;
uint64_t seqnum = 0;
uint64_t lastrec = 0;
FILE* output_file = NULL;
FILE* logger = NULL;
bool timestamps = false;
struct data_pack** packet_table = NULL;
struct alpha_queue* rpack_garbage;
struct alpha_queue* follower_queue;
pthread_cond_t follower_queue_nonempty = PTHREAD_COND_INITIALIZER;
struct alpha_queue* display_buffer_queue;
pthread_cond_t display_buffer_queue_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t window_nonfull = PTHREAD_COND_INITIALIZER;
pthread_cond_t window_nonempty = PTHREAD_COND_INITIALIZER;


volatile uint64_t last_rec = 0;
volatile uint64_t trail = 0;
int loopnum = 0;
struct recv_pack* receive_ar[BUFFER_SIZE];

// Internal node to store peer_info, related eu_sock, and use in linked lists
struct peer_node
{
  struct peer_info peer_info;
  struct eu_socket* eu_sock;
  struct peer_node* next;
};

struct recv_pack
{
  size_t length;
  struct data_pack dpack;
};


// Wraps peer_info data into a peer_node
// Attempts to open a eu_socket on that peer
struct peer_node* peer_node(struct peer_info node_params)
{
  struct peer_node* pn = (struct peer_node*)malloc(sizeof(struct peer_node));
  pn->peer_info = node_params;
  pn->eu_sock = eu_socket(EU_PUSH);
  if(eu_connect(pn->eu_sock, node_params.addr, node_params.port))
  {
    print_error("Failed to conncet to peer %s", node_params.pid);
    eu_close(pn->eu_sock);
    return NULL;
  }
  pn->next = NULL;
  return pn;
}

// Traverses peer_node linked list and closes sockets/frees memory
void close_all_peer_sockets()
{
  struct peer_node* node = downstream_peers;
  struct peer_node* last;
  while(node != NULL)
  {
    last = node;
    eu_close(node->eu_sock);
    free(node->eu_sock);
    node = node->next;
    free(last);
  }
  return;
}

// Helper function that pushes len data in buf to all peers
void push_data_to_peers(char* buf, size_t len)
{
  struct peer_node* node = downstream_peers;
  int rc;
  while(node != NULL)
  {
    // Blocking send
    rc = eu_send(node->eu_sock, buf, len, 0);
    if(rc < len)
    {
      print_error("Failed to send data to peer %s", node->peer_info.pid);
      // If data push fails, don't return.
      // but continue to try to push data to other peers
    }
    node = node->next;
  }
  return;
}

// Helper function called upon FEED being recv'd by statusThread
// adds new peer to end of linked list
void add_downstream_peer(struct peer_node* new_peer)
{
  if(downstream_peers == NULL)
    downstream_peers = new_peer;
  else
  {
    struct peer_node* currNode = downstream_peers;
    while(currNode->next != NULL)
      currNode = currNode->next;
    currNode->next = new_peer;
  }
  num_downstream_peers++;
  return;
}

// Traverses linked list of peers and closes that peers socket/frees memory
void drop_downstream_peer(struct peer_node* new_peer)
{
  if(downstream_peers == NULL)
    return;
  else
  {
    struct peer_node* currNode = downstream_peers;
    while(currNode->next != NULL)
    {
      if(currNode->next->peer_info.pid == new_peer->peer_info.pid)
      {
        struct peer_node* targetNode = currNode->next;
        currNode->next = currNode->next->next;
        eu_close(targetNode->eu_sock);
        free(targetNode);
        return;
      }
      currNode = currNode->next;
    }
  }
  return;
}

struct recv_pack* alloc_rpack(void)
{
  struct recv_pack* rpack = alpha_queue_pop(rpack_garbage);
  if (NULL == rpack)
  {
    rpack = malloc(sizeof *rpack);
    if (NULL == rpack)
      print_error ("out of memory");
  }

  return rpack;
}

void free_rpack(struct recv_pack* rpack)
{
  if (!alpha_queue_push(rpack_garbage, rpack))
    print_error ("absurd failure");
}

void* recvThread(void* arg)
{
  for(;;)
  {
    struct recv_pack* rpack = alloc_rpack();
    char from_addr[EU_ADDRSTRLEN];
    char from_port[EU_PORTSTRLEN];

    if(NULL == rpack)
      return NULL;

    // Do a blocking receive
    if(0 < (rpack->length = eu_recv(upstream_eu_sock, &rpack->dpack,
                                    EU_PACKETLEN, 0, from_addr, from_port)))
    {
      struct recv_pack* rpack_copy = alloc_rpack();

      if(NULL == rpack_copy)
        return NULL;

      // we're pushing to two queues so we need two copies
      memcpy(rpack_copy, rpack, sizeof *rpack);

      if(!alpha_queue_push(follower_queue, rpack))
      {
        print_error("out of memory");
        return NULL;
      }
      // alert follower thread that we've pushed something
      pthread_cond_broadcast(&follower_queue_nonempty);

      if(!alpha_queue_push(display_buffer_queue, rpack_copy))
      {
        print_error("out of memory");
        return NULL;
      }
      // alert display buffer thread that we've pushed something.
      pthread_cond_broadcast(&display_buffer_queue_nonempty);
    }
    else
    {
      free_rpack(rpack);
      print_error ("End of stream?");
      return NULL;
    }
  }
}

void* followerThread(void* arg)
{
  pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

  for(;;)
  {
    struct recv_pack* rpack = alpha_queue_pop(follower_queue);
    if(NULL == rpack)
    {
      // wait for the recv thread to push this queue
      pthread_mutex_lock(&fastmutex);
      while(NULL == (rpack = alpha_queue_pop(follower_queue)))
        pthread_cond_wait(&follower_queue_nonempty, &fastmutex);
      pthread_mutex_unlock(&fastmutex);
    }

    if(NULL == rpack)
    {
      print_error("pthreads failed me");
      return NULL;
    }

    push_data_to_peers((char*) &rpack->dpack, rpack->length);

    free_rpack(rpack);
  }
}

void* displayBufferThread(void* arg)
{
  pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

  for(;;)
  {
    uint64_t current_seqnum;
    uint64_t window_index;

    struct recv_pack* rpack = alpha_queue_pop(display_buffer_queue);
    if(NULL == rpack)
    {
      // wait for the recv thread to push this queue
      pthread_mutex_lock(&fastmutex);
      while(NULL == (rpack = alpha_queue_pop(display_buffer_queue)))
        pthread_cond_wait(&display_buffer_queue_nonempty, &fastmutex);
      pthread_mutex_unlock(&fastmutex);
    }

    if(NULL == rpack)
    {
      print_error("pthreads failed me");
      return NULL;
    }

    current_seqnum = rpack->dpack.seqnum;
    window_index = current_seqnum % BUFFER_SIZE;

    if(0 == trail || 0 == last_rec)
    {
      assert(trail == last_rec);
      trail = last_rec = current_seqnum;
      print_error ("first packet has seqnum: %lu", current_seqnum);
    }

    if(last_rec < BUFFER_SIZE || last_rec - BUFFER_SIZE < current_seqnum)
    {
      // make sure that the window is empty enough
      while(trail + BUFFER_SIZE < current_seqnum)
      {
        pthread_mutex_lock(&fastmutex);
        while(trail + BUFFER_SIZE < current_seqnum)
          pthread_cond_wait(&window_nonfull, &fastmutex);
        pthread_mutex_unlock(&fastmutex);
      }

      receive_ar[window_index] = rpack;

      if(last_rec < current_seqnum)
      {
        last_rec = current_seqnum;

        // alert display thread that the window is no longer empty
        pthread_cond_broadcast(&window_nonempty);
      }
      else
      {
        // otherwise we've received a 'late' packet, any cond-waiter won't care.
      }
    }
    else
    {
      fprintf(logger, "Didn't put %lu in array\n", window_index);
    }
  }
}

void* displayThread(void* arg)
{
  pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

  for(;;)
  {
    struct recv_pack* rpack;

    // wait for this condition to become false
    while(last_rec <= trail + (BUFFER_SIZE >> 1))
    {
      pthread_mutex_lock(&fastmutex);
      while(last_rec <= trail + (BUFFER_SIZE >> 1))
        pthread_cond_wait(&window_nonempty, &fastmutex);
      pthread_mutex_unlock(&fastmutex);

      // we must loop again just in case the window isnt full 'enough'
    }

    rpack = receive_ar[trail % BUFFER_SIZE];
    if(NULL == rpack)
    {
      fprintf(logger, "dropped sequence: %lu, last received: %lu\n", trail, last_rec);
      fflush(logger);
    }
    else
    {
      fwrite(rpack->dpack.data, 1, rpack->length - sizeof rpack->dpack.seqnum, output_file);
      fflush(output_file);

      receive_ar[trail % BUFFER_SIZE] = NULL;
      free_rpack(rpack);
    }

    trail++;

    // alert display buffer thread that the window is no longer full
    pthread_cond_broadcast(&window_nonfull);
  }
}

void* statusThread(void* arg)
{
  while(1)
  {
    // Receive message from the source
    message_struct* msg = fn_rcvmsg(source_zmq_sock);
    if(!msg)
      print_error("Error: Status thread received null msg\n");

    print_error("Received Message: ");
    if(msg->type == FEED_NODE)
    {
      struct peer_node* pn = peer_node(msg->node_params);
      print_error("FEED_NODE\n");
      print_error("Adding downstream peer %s\n", msg->node_params.pid);
      pthread_mutex_lock(&downstream_peers_mutex);
      add_downstream_peer(pn);
      pthread_mutex_unlock(&downstream_peers_mutex);
    }
    else if(msg->type == DROP_NODE)
    {
      struct peer_node* pn = peer_node(msg->node_params);
      print_error("DROP_NODE\n");
      print_error("Dropping downstream peer %s\n", msg->node_params.pid);
      pthread_mutex_lock(&downstream_peers_mutex);
      drop_downstream_peer(pn);
      pthread_mutex_unlock(&downstream_peers_mutex);
    }
    else if(msg->type == FOLLOW_NODE)
    {
      struct peer_info pi;
      memcpy(&pi, &(msg->node_params), sizeof(struct peer_info));
      print_error("FOLLOW_NODE\n");
      print_error("Changing upstream peer %s\n", pi.pid);
      // XXX the action taken below is not fulfilled by the following 3 lines
      //     rather, the data thread should be updated to stop trashing udp
      //       packets from `pi'. for now its O.K. to do nothing.
      //pthread_mutex_lock(&upstream_peer_mutex);
      //change_upstream_peer(&pi);
      //pthread_mutex_unlock(&upstream_peer_mutex);
    }
    else
    {
    }
  }
}

int main(int argc, char* argv[])
{
  int i;
  struct bootstrap* b;
  void* sock = NULL;
  char* lobby_token = NULL;

  if(argc < 4)
  {
    printf("Usage:\n");
    printf("eyeunitefollower <lobby token> <listen port> <bandwidth> [output file] [--debug]\n");
    return -1;
  }
  lobby_token = argv[1];
  memcpy(my_peer_info.port, argv[2], EU_PORTSTRLEN);
  my_peer_info.peerbw = atoi(argv[3]);
  output_file = stdout;
  timestamps = false;
  if(argc >= 5)
  {
    if(strcmp(argv[4], "--debug") == 0)
      timestamps == true;
    else
      output_file = fopen(argv[4], "w+");
  }
  if(argc >= 6 && (strcmp(argv[5], "--debug") == 0))
    timestamps = true;

  // Bootstrap
  bootstrap_global_init();
  if(!(b = bootstrap_init(APP_ENGINE, my_peer_info.port, my_peer_info.pid,
                          my_peer_info.addr)))
  {
    print_error("Bootstrap call: Failed intitializing bootstrap!\n");
    return 1;
  }
  if(bootstrap_lobby_join(b, lobby_token))
  {
    print_error("Bootstrap call: Failed joining lobby %s\n", lobby_token);
    return 1;
  }
  if(bootstrap_lobby_get_source(b, &source_peer))
  {
    print_error("Bootstrap call: Failed to get source\n");
    return 1;
  }

  // Finish initialization
  downstream_peers = NULL;
  num_downstream_peers = 0;
  logger = fopen("log.txt", "ab");
  
  // Initialize to NULL
  for (last_rec = 0; last_rec < BUFFER_SIZE; last_rec++)
  {
    receive_ar[last_rec] = NULL;
  }
  last_rec = 1;
  trail = 1;


  // Initiate connection to source
  print_error ("source pid: %s", source_peer.pid);
  print_error ("source ip: %s:%s", source_peer.addr, source_peer.port);
  char temp[EU_ADDRSTRLEN*4];
  snprintf (temp, EU_ADDRSTRLEN*4, "tcp://%s:%s", source_peer.addr,
            source_peer.port);
  source_zmq_sock = fn_initzmq (my_peer_info.pid, temp);
  fn_sendmsg(source_zmq_sock, REQ_JOIN, &my_peer_info);

  // Bind to socket that will receive data (can be used without re-binding for
  // the duration of the program).
  upstream_eu_sock = eu_socket(EU_PULL);
  
  eu_bind(upstream_eu_sock, NULL, my_peer_info.port);
  if (upstream_eu_sock == NULL)
  {
    print_error ("Couldn't bind to socket!");
    return 1;
  }

  rpack_garbage = alpha_queue_new ();
  assert (rpack_garbage);
  follower_queue = alpha_queue_new ();
  assert (follower_queue);
  display_buffer_queue = alpha_queue_new ();
  assert (display_buffer_queue);

  pthread_t recv_thread;
  pthread_t follower_thread;
  pthread_t display_buffer_thread;
  pthread_t display_thread;
  pthread_t status_thread;

  // Start status thread
  pthread_create(&recv_thread, NULL, recvThread, NULL);
  pthread_create(&follower_thread, NULL, followerThread, NULL);
  pthread_create(&display_buffer_thread, NULL, displayBufferThread, NULL);
  pthread_create(&display_thread, NULL, displayThread, NULL);
  pthread_create(&status_thread, NULL, statusThread, NULL);


  // Clean up at termination
  pthread_join(recv_thread, NULL);
  pthread_join(follower_thread, NULL);
  pthread_join(display_buffer_thread, NULL);
  pthread_join(display_thread, NULL);
  pthread_join(status_thread, NULL);

  // Close sockets and free memory
  alpha_queue_free(display_buffer_queue);
  alpha_queue_free(follower_queue);
  bootstrap_cleanup(b);
  bootstrap_global_cleanup();
  fn_closesocket(source_zmq_sock);
  close_all_peer_sockets();
  if(output_file != stdout)
    fclose(output_file);

  return 0;
}
