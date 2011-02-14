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
  struct tnode* temp_node = malloc (sizeof(struct tnode));
  temp_node->pid = 222;
  temp_node->latency = 1;
  temp_node->upload = 2;
  temp_node->child = 3;
  temp_node->max = 4;
  temp_node->joinable = 5;
  fn_sendmsg(sock, REQ_JOIN, temp_node);

  // LOOP UNTIL MESSAGE RECEIVED
  message_struct* msg = NULL;
  while (msg == NULL) {
	msg = fn_rcvmsg(sock);
  }

  // PRINT THE MESSAGE
  printf("Message received\n");
  printf("Type:     %s\n", sn_mtype_to_string(msg->type));
  printf("PID:      %i\n", msg->node_params.pid);
  printf("LATENCY:  %i\n", msg->node_params.latency);
  printf("UPLOAD:   %i\n", msg->node_params.upload);
  printf("CHILD:    %i\n", msg->node_params.child);
  printf("MAX:      %i\n", msg->node_params.max);
  printf("JOINABLE: %i\n", msg->node_params.joinable);

  fn_closesocket(sock);
}
