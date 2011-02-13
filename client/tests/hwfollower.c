/*
  FOLLOWER NODE TESTING
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "followernode.h"

int main(void)
{
  // Create a port
  const char* pid = "222";
  const char* endpoint = "tcp://127.0.0.1:22222";
  const char* connect_to = "tcp://127.0.0.1:55555";

  void* sock = fn_initzmq (endpoint, pid, connect_to);

  // Send message
  tnode* temp_node = malloc(sizeof(tnode));
  fn_sendmsg(sock, "111", "REQ_JOIN", temp_node);

  fn_closesocket(sock);
}
