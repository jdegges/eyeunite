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
struct peer_info my_peer_info;
uint64_t seqnum = 0;
uint64_t lastrec = 0;
FILE* output_file = NULL;
FILE* logger = NULL;
bool timestamps = false;
struct data_pack** packet_table = NULL;
struct alpha_queue* fifo_queue;
struct alpha_queue* follower_queue;
pthread_cond_t follower_queue_nonempty = PTHREAD_COND_INITIALIZER;
struct alpha_queue* mpack_garbage;


volatile uint64_t last_rec = 0;
volatile uint64_t trail = 0;
int loopnum = 0;
struct media_pack* receive_ar[BUFFER_SIZE];

// Internal node to store peer_info, related eu_sock, and use in linked lists
struct peer_node
{
  struct peer_info peer_info;
  struct eu_socket* eu_sock;
  struct peer_node* next;
};

struct media_pack
{
  struct data_pack data;
  int len;
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
  assert(len > 0);
  while(node != NULL)
  {
    // Blocking send
    rc = eu_send(node->eu_sock, buf, len, 0);
    if(rc < (int)len)
    {
      print_error("Failed to send data to peer %s", node->peer_info.pid);
      // If data push fails, don't return.
      // but continue to try to push data to other peers
      
      // Send request to drop node
      fn_sendmsg(source_zmq_sock, REM_NODE, &(node->peer_info));
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
    if(!strncmp(currNode->peer_info.pid, new_peer->peer_info.pid,
                EU_TOKENSTRLEN))
    {
      downstream_peers = currNode->next;
      eu_close(currNode->eu_sock);
      free(currNode);
      return;
    }
    while(currNode->next != NULL)
    {
      if(!strncmp(currNode->next->peer_info.pid, new_peer->peer_info.pid,
                  EU_TOKENSTRLEN))
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

struct media_pack* alloc_mpack(void)
{
  struct media_pack* mpack = alpha_queue_pop(mpack_garbage);
  if (NULL == mpack)
  {
    mpack = malloc(sizeof *mpack);
    if (NULL == mpack)
      print_error ("out of memory");
  }

  return mpack;
}

void free_mpack(struct media_pack* mpack)
{
  if (!alpha_queue_push(mpack_garbage, mpack))
    print_error ("absurd failure");
}

// Listens on upstream eu_socket for incoming UDP data to push to peers
// Delays playback by a short amount, and has a timeout period for missing packets
void* dataThread(void* arg)
{
  ssize_t length;
  char buf[EU_PACKETLEN];
  while(1)
  {
    char from_addr[EU_ADDRSTRLEN];
    char from_port[EU_PORTSTRLEN];
    struct media_pack* media = alloc_mpack();

    if(NULL == media)
    {
      print_error ("out of memory?");
      return NULL;
    }

    // Blocking recv
    if(0 < (media->len = eu_recv(upstream_eu_sock,&media->data, EU_PACKETLEN,
                                 0, from_addr, from_port)))
    {
      // Converts char string buffer to packet struct
      media->len -= sizeof(uint64_t);
      
      // copy data to send to follower thread
      struct media_pack* forward_packet = alloc_mpack();
      if(NULL == forward_packet)
      {
        print_error("out of memory");
        return NULL;
      }

      memcpy(forward_packet, media, sizeof *media);
      if(!alpha_queue_push(follower_queue, forward_packet))
      {
        print_error("out of memory");
        return NULL;
      }
      // alert follower thread that we've pushed something
      pthread_cond_broadcast(&follower_queue_nonempty);
      
      uint64_t tempseqnum = media->data.seqnum;
      uint64_t temp_index = tempseqnum % BUFFER_SIZE;
      if (last_rec < BUFFER_SIZE || last_rec - BUFFER_SIZE < tempseqnum)
      {
        if (tempseqnum > last_rec)
          last_rec = tempseqnum;
        while (trail + BUFFER_SIZE < tempseqnum)
          assert(sched_yield()==0);
        receive_ar[temp_index] = media;
      }
      else
      {
        fprintf(logger, "Didn't put %lu in array\n", temp_index);
        free_mpack(media);
      }
    }
    else
    {
      print_error("absurd failure");
      return NULL;
    }
  }
}

void* bufferThread(void* arg)
{
  while(1)
  {
    if(last_rec > (BUFFER_SIZE >> 1))
    {
      struct media_pack* media;
      while (trail + (BUFFER_SIZE >> 1) < last_rec)
      {
        media = receive_ar[trail % BUFFER_SIZE];
        if (media != NULL)
        {
          alpha_queue_push(fifo_queue, media);
          receive_ar[trail % BUFFER_SIZE] = NULL;
        }
        else
        {
          fprintf(logger, "dropped sequence: %lu, last received: %lu\n", trail, last_rec);
        }
        trail++;
      }
      sched_yield();
    }
  }
}

void* outputThread(void* arg)
{
  uint64_t last_pushed = 0;
  struct media_pack* media;
  
  for (;;)
  {
    if ((media = alpha_queue_pop(fifo_queue)) != NULL)
    {
      if (last_pushed < media->data.seqnum)
      {
        fwrite(media->data.data, 1, media->len, output_file);
        fflush(output_file);
        last_pushed = media->data.seqnum;
      }
      free_mpack(media);
    }
  }
}

void* followerThread(void* arg)
{
  pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;

  for(;;)
  {
    struct media_pack* forward_packet = alpha_queue_pop(follower_queue);
    if(NULL == forward_packet)
    {
      // wait for the recv thread to push this queue
      pthread_mutex_lock(&fastmutex);
      while(NULL == (forward_packet = alpha_queue_pop(follower_queue)))
        pthread_cond_wait(&follower_queue_nonempty, &fastmutex);
      pthread_mutex_unlock(&fastmutex);
    }

    if(NULL == forward_packet)
    {
      print_error("pthreads failed me");
      return NULL;
    }

    push_data_to_peers((char*) &(forward_packet->data), forward_packet->len + sizeof(uint64_t));

    free_mpack(forward_packet);
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
    printf("eyeunitefollower <lobby token> <listen port> <bandwidth> [output file] [--debug]");
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
  fifo_queue = alpha_queue_new();
  assert (fifo_queue);
  follower_queue = alpha_queue_new ();
  assert (follower_queue);
  mpack_garbage = alpha_queue_new();
  assert (mpack_garbage);
  
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

  pthread_t status_thread;
  pthread_t data_thread;
  pthread_t buffer_thread;
  pthread_t follower_thread;
  pthread_t output_thread;

  // Start status thread
  pthread_create(&status_thread, NULL, statusThread, NULL);
  pthread_create(&data_thread, NULL, dataThread, NULL);
  pthread_create(&buffer_thread, NULL, bufferThread, NULL);
  pthread_create(&follower_thread, NULL, followerThread, NULL);
  pthread_create(&output_thread, NULL, outputThread, NULL);


  // Clean up at termination
  pthread_join(status_thread, NULL);
  pthread_join(data_thread, NULL);
  pthread_join(buffer_thread, NULL);
  pthread_join(follower_thread, NULL);
  pthread_join(output_thread, NULL);

  // Close sockets and free memory
  bootstrap_cleanup(b);
  bootstrap_global_cleanup();
  fn_closesocket(source_zmq_sock);
  close_all_peer_sockets();
  alpha_queue_free(fifo_queue);
  if(output_file != stdout)
    fclose(output_file);

  return 0;
}
