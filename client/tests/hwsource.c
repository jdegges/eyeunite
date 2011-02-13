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
  const char* endpoint = "tcp://127.0.0.1:55555";
  void* sock = sn_initzmq (endpoint, pid);

  // Loop until message is received
  message_struct* msg = sn_rcvmsg (sock);
  while (msg == NULL) {
	msg = sn_rcvmsg(sock);
  }

  // Print the type of message
  printf("%s",msg->type);

  sn_closesocket(sock);
}
