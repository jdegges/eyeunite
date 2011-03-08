#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>
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


volatile uint64_t last_rec = 0;
volatile uint64_t trail = 0;
int loopnum = 0;
struct data_pack* receive_ar[BUFFER_SIZE];

// Internal node to store peer_info, related eu_sock, and use in linked lists
struct peer_node
{
  struct peer_info peer_info;
  struct eu_socket* eu_sock;
  struct peer_node* next;
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
};

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


// Listens on upstream eu_socket for incoming UDP data to push to peers
// Delays playback by a short amount, and has a timeout period for missing packets
void* dataThread(void* arg)
{
  ssize_t len;
  char buf[EU_PACKETLEN];
  while(1)
  {
    char from_addr[EU_ADDRSTRLEN];
    char from_port[EU_PORTSTRLEN];

    // Blocking recv
    if(upstream_eu_sock && (len = eu_recv(upstream_eu_sock, buf, EU_PACKETLEN,
                                          0, from_addr, from_port)) > 0)
    {
      // Converts char string buffer to packet struct
      struct data_pack* packet = (struct data_pack*)malloc(sizeof(struct data_pack));
      memcpy(packet, buf, len);
      push_data_to_peers((char*)(packet), len);
      uint64_t tempseqnum = packet->seqnum;
      //print_error("SEQNUM %lu", tempseqnum);
      packet->seqnum = len - sizeof(uint64_t);
      uint64_t temp_index = tempseqnum % BUFFER_SIZE;
      if (last_rec < BUFFER_SIZE || last_rec - BUFFER_SIZE < tempseqnum)
      {
	while (trail + BUFFER_SIZE < tempseqnum)
	{
	  print_error("YIELDING");
	  assert(sched_yield()==0);
	  print_error("OUT");
	}
	receive_ar[temp_index] = packet;
      }
      else
      {
	char temp[EU_ADDRSTRLEN*4];
	int len = snprintf(temp, EU_ADDRSTRLEN*4, "Didn't put %lu in array\n", temp_index);
	fwrite(temp, 1, len, logger);
      }
      if (tempseqnum > last_rec)
	last_rec = tempseqnum;
    }
    else
    {
      print_error("absurd failure");
      return NULL;
    }
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

void* displayThread(void* arg)
{
  while(1)
  {
    if(last_rec > (BUFFER_SIZE >> 1))
    {
      
      struct data_pack* packet;
      while (trail + (BUFFER_SIZE >> 1) < last_rec)
      {
	packet = receive_ar[trail % BUFFER_SIZE];
	if (packet != NULL)
	{
	  fwrite(packet->data, 1, packet->seqnum, output_file);
	  fflush(output_file);
	  receive_ar[trail % BUFFER_SIZE] = NULL;
	  free(packet);
	}
	else
	{
	  char temp[EU_ADDRSTRLEN*4];
	  int len = snprintf(temp, EU_ADDRSTRLEN*4, "DROPPED SEQUENCE: %lu. LAST RECEIVED: %lu\n", trail, last_rec);
	  fwrite(temp ,1,len, logger);
	  fflush(logger);
	}
	trail++;
      }
      sched_yield();
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
  pthread_t display_thread;

  // Start status thread
  pthread_create(&status_thread, NULL, statusThread, NULL);
  pthread_create(&data_thread, NULL, dataThread, NULL);
  pthread_create(&display_thread, NULL, displayThread, NULL);


  // Clean up at termination
  pthread_join(status_thread, NULL);
  pthread_join(data_thread, NULL);
  pthread_join(display_thread, NULL);

  // Close sockets and free memory
  bootstrap_cleanup(b);
  bootstrap_global_cleanup();
  fn_closesocket(source_zmq_sock);
  close_all_peer_sockets();
  if(output_file != stdout)
    fclose(output_file);

  return 0;
}
