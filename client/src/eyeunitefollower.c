#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "bootstrap.h"
#include "followernode.h"
#include "tree.h"
#include "debug.h"

// Global variables for threads
struct peer_info upstream_peer;
struct peer_node* downstream_peers;
size_t num_downstream_peers;
pthread_mutex_t downstream_peers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t upstream_peer_mutex = PTHREAD_MUTEX_INITIALIZER;

struct peer_node
{
	struct peer_info peer_info;
	struct peer_node* next;
};

struct peer_node* peer_node(struct peer_info node_params)
{
	struct peer_node* pn = (struct peer_node*)malloc(sizeof(struct peer_node));
	pn->peer_info = node_params;
	pn->next = NULL;
	return pn;
};

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
				free(targetNode);
				return;
			}
			currNode = currNode->next;
		}
	}
	return;
}

void change_upstream_peer(struct peer_info up_peer)
{
	upstream_peer = up_peer;
}

struct status_thread_args
{
	void* sock;
};

void* statusThread(void* arg)
{
	struct status_thread_args* thread_args = (struct status_thread_args*)arg;
	message_struct* msg = NULL;
	msg = fn_rcvmsg(thread_args->sock);
	if(!msg)
		printf("Error: Status thread received null msg\n");
	if(msg->type == FEED_NODE)
	{
		struct peer_node* pn = peer_node(msg->node_params);
		printf("Adding downstream peer %s\n", msg->node_params.pid);
		pthread_mutex_lock(&downstream_peers_mutex);
		add_downstream_peer(pn);
		pthread_mutex_unlock(&downstream_peers_mutex);
	}
	else if(msg->type == DROP_NODE)
	{
		struct peer_node* pn = peer_node(msg->node_params);
		printf("Dropping downstream peer %s\n", msg->node_params.pid);
		pthread_mutex_lock(&downstream_peers_mutex);
		drop_downstream_peer(pn);
		pthread_mutex_unlock(&downstream_peers_mutex);
	}
	else if(msg->type == FOLLOW_NODE)
	{
		struct peer_info pi = msg->node_params;
		printf("Changing upstream peer %s\n", pi.pid);
		pthread_mutex_lock(&upstream_peer_mutex);
		change_upstream_peer(pi);
		pthread_mutex_unlock(&upstream_peer_mutex);
	}
	else
	{
	}
}

int main(int argc, char* argv[])
{
	int i;
	struct bootstrap* b;
  const char* endpoint = "tcp://*:55555";
	struct peer_info* source_info;
	char* lobby_token;
	void* sock;

	// Follower peer info variables
	struct peer_info my_peer_info;
	char my_pid[EU_TOKENSTRLEN];
	char* my_addr;
	int my_bw;
	int my_port;

	if(argc < 4)
	{
		printf("Usage:\n");
		printf("eyeunitefollower <lobby token> <listen port> <bandwidth>");
		return -1;
	}
	lobby_token = argv[1];
	my_port = atoi(argv[2]);
	my_bw = atoi(argv[3]);

	// Bootstrap
	if(!(b = bootstrap_init("http://eyeunite.appspot.com", 80, my_pid, my_addr)))
		printf("Failed intitializing bootstrap!\n");
	if(!(bootstrap_lobby_join(b, lobby_token)))
		printf("Failed joining lobby %s\n", lobby_token);
	if(!(bootstrap_get_source(b, source_info)))
		printf("Failed to get source\n");

	// Set my peer_info
	memcpy(my_peer_info.pid, my_pid, EU_TOKENSTRLEN);
	memcpy(my_peer_info.addr, my_addr, EU_TOKENSTRLEN);
	my_peer_info.port = my_port;
	my_peer_info.peerbw = my_bw;

	// Finish initialization
	downstream_peers = NULL;
	num_downstream_peers = 0;

	// Initiate connection to source
  sock = fn_initzmq (endpoint, source_info->pid);
	fn_sendmsg(sock, REQ_JOIN, &my_peer_info);

	pthread_t status_thread;

	// NOT USED YET
	// pthread_t data_rcv_thread;
	// pthread_t data_push_thread;
	// No display thread yet

	// Start status thread
	struct status_thread_args st_args;
	st_args.sock = sock;
	pthread_create(&status_thread, NULL, statusThread, &st_args);

	return 0;
}
