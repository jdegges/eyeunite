#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>

#include "bootstrap.h"
#include "followernode.h"
#include "tree.h"
#include "debug.h"
#include "easyudp.h"
#include "alpha_queue.h"
#include "time.h"

// Global variables for threads
struct peer_info* upstream_peer = NULL;
void* upstream_sock = NULL;
struct eu_socket* up_eu_sock = NULL;
struct peer_node* downstream_peers = NULL;
size_t num_downstream_peers = 0;
pthread_mutex_t downstream_peers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t upstream_peer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t packet_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
char my_pid[EU_TOKENSTRLEN];
char my_addr[EU_ADDRSTRLEN];
char my_port[EU_PORTSTRLEN];
int my_bw = 1;
int seqnum = -1;
FILE* output_file = NULL;
bool timestamps = false;

GHashTable* packet_table = NULL;

struct peer_node
{
  struct peer_info peer_info;
  struct eu_socket* eu_sock;
  struct peer_node* next;
};

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

void close_all_sockets()
{
  if(upstream_sock)
    fn_closesocket(upstream_sock);
  struct peer_node* node = downstream_peers;
  while(node != NULL)
  {
    eu_close(node->eu_sock);
    node = node->next;
  }
  return;
}

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
      // Don't return, but continue to try to push data to other peers
    }
    node = node->next;
  }
  return;
}

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

void change_upstream_peer(struct peer_info* up_peer)
{
  void* new_up_eu_sock = eu_socket(EU_PULL);
  eu_bind(new_up_eu_sock, my_addr, my_port);
  if(up_eu_sock != NULL)
    eu_close(up_eu_sock);
  up_eu_sock = new_up_eu_sock;
}

void* dataThread(void* arg)
{
  ssize_t len;
  char buf[EU_PACKETLEN];
  while(1)
  {
    // Blocking recv
    if(up_eu_sock && upstream_peer && (len = eu_recv(up_eu_sock, buf, EU_PACKETLEN, 0, upstream_peer->addr, upstream_peer->port)) > 0)
    {
      struct data_pack* packet = (struct data_pack*)malloc(sizeof(struct data_pack));
      memcpy(packet, buf, len);
      // Set the starting sequence number to the first packet that arrives
      if(seqnum < 0)
        seqnum = packet->seqnum;

      // Drops out of order packets that are behind the display thread
      if(!(packet->seqnum < seqnum))
      {
        pthread_mutex_lock(&packet_buffer_mutex);
        uint64_t seqnum = packet->seqnum;
        packet->seqnum = len;
        g_hash_table_insert(packet_table, &seqnum, packet);
        pthread_mutex_unlock(&packet_buffer_mutex);
      }

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
    message_struct* msg = fn_rcvmsg(upstream_sock);
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
      struct data_pack* packet;
      packet = g_hash_table_lookup(packet_table, &seqnum);
      if(packet != NULL)
      {
        // Remove packet from hash table buffer
        pthread_mutex_lock(&packet_buffer_mutex);
        g_hash_table_remove(packet_table, &seqnum);
        pthread_mutex_unlock(&packet_buffer_mutex);

        // "Display" packet
        char temp[EU_PACKETLEN*2];
        snprintf(temp, EU_PACKETLEN*2, "Packet [%lld]: %s", packet->seqnum, packet->data);
        if(timestamps)
          fwrite(temp, 1, EU_PACKETLEN*2, output_file);
        else
          fwrite(packet->data, 1, packet->seqnum, output_file);
        free(packet);
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
  char* lobby_token = NULL;
  void* sock = NULL;

  // Follower peer info variables
  struct peer_info* my_peer_info = NULL;

  if(argc < 4)
  {
    printf("Usage:\n");
    printf("eyeunitefollower <lobby token> <listen port> <bandwidth> [output file] [--debug]");
    return -1;
  }
  lobby_token = argv[1];
  memcpy(my_port, argv[2], EU_PORTSTRLEN);
  my_bw = atoi(argv[3]);
  output_file = stdout;
  timestamps = false;
  if(argc >= 5)
  {
    if(strcmp(argv[4], "--debug") == 0)
      timestamps == true;
    else
      output_file = fopen(argv[4], "ab");
  }
  if(argc >= 6 && (strcmp(argv[5], "--debug") == 0))
    timestamps = true;

  // Bootstrap
  bootstrap_global_init();
  upstream_peer = malloc(sizeof(struct peer_info));
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
  if(bootstrap_lobby_get_source(b, upstream_peer))
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
  packet_table = g_hash_table_new(g_int_hash, g_int_equal);

  // Initiate connection to source
  print_error ("source pid: %s", upstream_peer->pid);
  print_error ("source ip: %s:%s", upstream_peer->addr, upstream_peer->port);
  char temp[EU_ADDRSTRLEN*4];
  snprintf (temp, EU_ADDRSTRLEN*4, "tcp://%s:%s", upstream_peer->addr, upstream_peer->port);
  upstream_sock = fn_initzmq (my_pid, temp);
  fn_sendmsg(upstream_sock, REQ_JOIN, my_peer_info);
  
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

  close_all_sockets();
  if(output_file != stdout)
    fclose(output_file);

  bootstrap_cleanup(b);
  bootstrap_global_cleanup();
  free(my_peer_info);
  free(upstream_peer);
  return 0;
}
