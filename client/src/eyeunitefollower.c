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
	struct bootstrap* b;
	char* lobby_token = "5432123456";
	struct bootstrap_peer peers_list[MAX_PEERS];
	int i;
  const char* endpoint = "tcp://*:55555";
	char* source_pid;
	void* sock;

	if(argc >= 2)
		lobby_token = argv[1];

	//-- Fix this after bootstrap changes --

	if(!(b = bootstrap_init("http://eyeunite.appspot.com", 80)))
		printf("Failed intitializing bootstrap!\n");
	if(!(bootstrap_lobby_join(b, lobby_token)))
		printf("Failed joining lobby %s\n", lobby_token);
	size_t num_peers = MAX_PEERS;
	if(!(bootstrap_lobby_list(b, peers_list, num_peers)))
		printf("Failed to get peer list\n");

	//--

	// Initialization Step
	for(i = 0; i < num_peers; i++)
	{
		// Ping library? easyudp? -- Need to ask joey about that
	}
	// Report pings to source, for now, pretend we get it back

	// Fix these
	source_pid = "1";
	downstream_peers = NULL;
	num_downstream_peers = 0;

	// Initiate connection to source
  sock = fn_initzmq (endpoint, source_pid);

	pthread_t status_thread;
	pthread_t data_rcv_thread;
	pthread_t data_push_thread;
	// No display thread yet

	// Start status thread
	struct status_thread_args st_args;
	st_args.sock = sock;
	pthread_create(&status_thread, NULL, statusThread, &st_args);

	free(peers_list);
	return 0;
}
