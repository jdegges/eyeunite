/*
  SOURCE NODE TESTING
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sourcenode.h"

int main(void)
{
  // Create a port
  const char* pid = "111";
  const char* endpoint = "tcp://*:55555";
  void* sock = sn_initzmq (endpoint, pid);

  // Loop until message is received
  message_struct* msg = NULL;
  while (msg == NULL) {
	msg = sn_rcvmsg(sock);
  }

  // Print the type of message
  printf("\n\n");
  printf("Message received from follower\n");
  printf("From:     %s\n", msg->identity);
  printf("Type:     %s\n", sn_mtype_to_string(msg->type));
  printf("PID:      %s\n", msg->node_params.pid);
  printf("ADDRESS:  %s\n", msg->node_params.addr);
  printf("PORT:     %i\n", msg->node_params.port);
  printf("BW:       %i\n", msg->node_params.peerbw);
  printf("\n\n");

  // REPLY TO FOLLOWER
  struct peer_info* temp_node = malloc (sizeof(struct peer_info));
  memcpy(temp_node->pid, "111", 4);
  memcpy(temp_node->addr, "127.0.0.1", 10);
  temp_node->port = 3333;
  temp_node->peerbw = 33;
  sn_sendmsg(sock, msg->identity, FOLLOW_NODE, temp_node);

  sleep(1);

  sn_closesocket(sock);
}
