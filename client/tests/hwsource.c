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
  printf("Message received\n");
  printf("From:     %s\n", msg->identity);
  printf("Type:     %s\n", sn_mtype_to_string(msg->type));
  printf("PID:      %i\n", msg->node_params.pid);
  printf("LATENCY:  %i\n", msg->node_params.latency);
  printf("UPLOAD:   %i\n", msg->node_params.upload);
  printf("CHILD:    %i\n", msg->node_params.child);
  printf("MAX:      %i\n", msg->node_params.max);
  printf("JOINABLE: %i\n", msg->node_params.joinable);

  // REPLY TO FOLLOWER
  struct tnode* temp_node = malloc (sizeof(struct tnode));
  temp_node->pid = 333;
  temp_node->latency = 5;
  temp_node->upload = 4;
  temp_node->child = 3;
  temp_node->max = 2;
  temp_node->joinable = 1;
  sn_sendmsg(sock, msg->identity, FOLLOW_NODE, temp_node);

  sleep(1);

  sn_closesocket(sock);
}
