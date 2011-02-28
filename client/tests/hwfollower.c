/*
  FOLLOWER NODE TESTING
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sourcenode.h"
#include "followernode.h"

int main(void)
{
  // CREATE A PORT
  const char* pid = "222";
  const char* connect_to = "tcp://127.0.0.1:55555";

  void* sock = fn_initzmq (pid, connect_to);

  // SEND MESSAGE
  struct peer_info* temp_node = malloc (sizeof(struct peer_info));
  memcpy(temp_node->pid, "222", 4);
  memcpy(temp_node->addr, "127.0.0.1", 10);
  memcpy(temp_node->port, "444", 4);
  temp_node->peerbw = 44;
  fn_sendmsg(sock, REQ_JOIN, temp_node);

  // LOOP UNTIL MESSAGE RECEIVED
  message_struct* msg = NULL;
  while (msg == NULL) {
	msg = fn_rcvmsg(sock);
  }

  // PRINT THE MESSAGE
  printf("Message received from source\n");
  printf("Type:            %s\n", sn_mtype_to_string(msg->type));
  printf("FOLLOW PID:      %s\n", msg->node_params.pid);
  printf("FOLLOW ADDRESS:  %s\n", msg->node_params.addr);
  printf("FOLLOW PORT:     %s\n", msg->node_params.port);
  printf("BW:              %i\n", msg->node_params.peerbw);
  printf("\n\n");

  fn_closesocket(sock);
}
