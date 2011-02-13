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
  const char* connect_to = "tcp://127.0.0.1:55555";

  void* sock = fn_initzmq (pid, connect_to);

  // Send message
  struct tnode* temp_node = calloc(1, sizeof(struct tnode));
  memset (temp_node, "A", sizeof (struct tnode)-1);
  fn_sendmsg(sock, "REQ_JOIN", temp_node);

  sleep (3);

  fn_closesocket(sock);
}
