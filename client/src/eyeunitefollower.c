#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "bootstrap.h"
#include "followernode.h"
#include "tree.h"
#include "debug.h"
#include "easyudp.h"
#include "alpha_queue.h"
#include "time.h"

#define MAP_SIZE 1024

// Global variables for threads
struct peer_info* upstream_peer = NULL;
struct peer_info* source_peer = NULL;
void* source_zmq_sock = NULL;
struct eu_socket* up_eu_sock = NULL;
struct peer_node* downstream_peers = NULL;
size_t num_downstream_peers = 0;
pthread_mutex_t downstream_peers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t upstream_peer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t packet_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
char* my_pid = NULL;
char* my_addr = NULL;
char* my_port = NULL;
int my_bw = 1;
int seqnum = -1;
FILE* output_file = NULL;
bool timestamps = false;
struct data_pack** packet_table = NULL;

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

// Called by statusThread upon recving FOLLOW msg
// * Binds new eu_socket for the new upstream peer
// * Closes any open upstream sockets and frees memory
void change_upstream_peer(struct peer_info* up_peer)
{
  void* new_up_eu_sock = eu_socket(EU_PULL);
  eu_bind(new_up_eu_sock, my_addr, my_port);
  if(up_eu_sock != NULL)
  {
    eu_close(up_eu_sock);
    free(up_eu_sock);
  }
  if(upstream_peer != NULL)
    free(upstream_peer);
  up_eu_sock = new_up_eu_sock;
  upstream_peer = up_peer;
}

// Listens on upstream eu_socket for incoming UDP data to push to peers
// Delays playback by a short amount, and has a timeout period for missing packets
void* dataThread(void* arg)
{
  ssize_t len;
  char buf[EU_PACKETLEN];
  while(1)
  {
    // Blocking recv
    if(up_eu_sock && (len = eu_recv(up_eu_sock, buf, EU_PACKETLEN, 0, upstream_peer->addr, upstream_peer->port)) > 0)
    {
      // Converts char string buffer to packet struct
      struct data_pack* packet = (struct data_pack*)malloc(sizeof(struct data_pack));
      memcpy(packet, buf, len);
      print_error("Recieved packet %llu", packet->seqnum);

      // Set the starting sequence number to the first packet that arrives
      if(seqnum < 0)
        seqnum = packet->seqnum;

      // Drops out of order packets that are behind the display thread
      if(!(packet->seqnum < seqnum))
      {
        pthread_mutex_lock(&packet_buffer_mutex);
        packet_table[packet->seqnum%MAP_SIZE] = packet;
        pthread_mutex_unlock(&packet_buffer_mutex);
      }

      // Pushes all packets to downstream peers
      pthread_mutex_lock(&downstream_peers_mutex);
      push_data_to_peers(buf, len);
      pthread_mutex_unlock(&downstream_peers_mutex);
    }
  }
}

void* statusThread(void* arg)
{
  while(1)
  {
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
      pthread_mutex_lock(&upstream_peer_mutex);
      change_upstream_peer(&pi);
      pthread_mutex_unlock(&upstream_peer_mutex);
    }
    else
    {
    }
  }
}

void* displayThread(void* arg)
{
  bool sleepOnce = true;
  int delay_ms = 2; // Delay before "playback"
  int timeout_ms = 20; // Stub value, replace later with better timeout interval
  clock_t start;
  while(1)
  {
    if(seqnum >= 0 && g_hash_table_size(packet_table) > 0)
    {
      if(sleepOnce)
      {
        sleep(delay_ms);
        sleepOnce = false;
      }
      if(packet_table[seqnum%MAP_SIZE] != NULL)
      {
        // Remove packet from hash table buffer
        pthread_mutex_lock(&packet_buffer_mutex);
        struct data_pack* packet = packet_table[seqnum%MAP_SIZE];
        packet_table[seqnum%MAP_SIZE] = NULL;
        pthread_mutex_unlock(&packet_buffer_mutex);

        // "Display" packet
        char* temp = malloc(EU_PACKETLEN*2*sizeof(char));
        snprintf(temp, EU_PACKETLEN*2, "Packet [%lld]: %s", packet->seqnum, packet->data);
        if(timestamps)
          fwrite(temp, 1, EU_PACKETLEN*2, output_file);
        else
          fwrite(packet->data, 1, packet->seqnum, output_file);
        free(packet);
        free(temp);
        start = clock();
        seqnum++;
      }
      else if(clock() > start + timeout_ms * CLOCKS_PER_SEC * 1000);
      {
        start = clock();
        seqnum++;
      }
    }
    else
      start = clock();
    usleep (50000);
  }
}

int main(int argc, char* argv[])
{
  int i;
  struct bootstrap* b;
  void* sock = NULL;
  char* lobby_token = NULL;

  my_pid = malloc(EU_TOKENSTRLEN*sizeof(char));
  my_port = malloc(EU_PORTSTRLEN*sizeof(char));
  my_addr = malloc(EU_ADDRSTRLEN*sizeof(char));

  // Follower peer info variables
  struct peer_info* my_peer_info = NULL;

  if(argc < 4)
  {
    printf("Usage:\n");
    printf("eyeunitefollower <lobby token> <listen port> <bandwidth> [output file] [--debug]");
    return -1;
  }
  lobby_token = argv[1];
  my_port = argv[2];
  my_bw = atoi(argv[3]);
  output_file = stdout;
  timestamps = false;
  if(argc >= 5)
  {
    if(strcmp(argv[4], "--debug") == 0)
      timestamps == true;
    else
      output_file = fopen(argv[4], "w");
  }
  if(argc >= 6 && (strcmp(argv[5], "--debug") == 0))
    timestamps = true;

  // Bootstrap
  bootstrap_global_init();
  source_peer = malloc(sizeof(struct peer_info));
  if(!(b = bootstrap_init(APP_ENGINE, my_port, my_pid, my_addr)))
  {
    print_error("Bootstrap call: Failed intitializing bootstrap!\n");
    return 1;
  }
  if(bootstrap_lobby_join(b, lobby_token))
  {
    print_error("Bootstrap call: Failed joining lobby %s\n", lobby_token);
    return 1;
  }
  if(bootstrap_lobby_get_source(b, source_peer))
  {
    print_error("Bootstrap call: Failed to get source\n");
    return 1;
  }

  // Set my peer_info
  my_peer_info = malloc (sizeof *my_peer_info);
  memcpy(my_peer_info->pid, my_pid, EU_TOKENSTRLEN);
  memcpy(my_peer_info->addr, my_addr, EU_ADDRSTRLEN);
  memcpy(my_peer_info->port, my_port, EU_PORTSTRLEN);
  my_peer_info->peerbw = my_bw;
  // Finish initialization
  downstream_peers = NULL;
  num_downstream_peers = 0;
  packet_table = malloc(MAP_SIZE*sizeof(struct data_pack*));
  for(i = 0; i < MAP_SIZE; i++)
    packet_table[i] = NULL;

  // Initiate connection to source
  print_error ("source pid: %s", source_peer->pid);
  print_error ("source ip: %s:%s", source_peer->addr, source_peer->port);
  char* temp = malloc(EU_ADDRSTRLEN*4*sizeof(char));
  snprintf (temp, EU_ADDRSTRLEN*4, "tcp://%s:%s", source_peer->addr, source_peer->port);
  source_zmq_sock = fn_initzmq (my_pid, temp);
  fn_sendmsg(source_zmq_sock, REQ_JOIN, my_peer_info);
  
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
  free(temp);
  free(my_peer_info);
  free(source_peer);
  free(my_pid);
  free(my_port);
  free(my_addr);
  fn_closesocket(source_zmq_sock);
  if(source_zmq_sock)
    free(source_zmq_sock);
  close_all_peer_sockets();
  if(output_file != stdout)
    fclose(output_file);

  return 0;
}
