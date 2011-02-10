#include "sourcenode.h"
#include "debug.h"

#include <stdlib.h>
#include <zmq.h>
#include <assert.h>

void
test(void)
{
  const char* endpoint = "tcp://127.0.0.1:5555";
  void* sock = sn_initzmq (endpoint);
  printf("Test\n");
}

void*
sn_initzmq (const char* endpoint) {
  /* THIS INIT ALLOWS FOR SERVICE TO RECEIVE    */
  /* REQUESTS FROM MULTIPLE CLIENTS AND RESPOND */
  /* TO THE CLIENT IT RECEIVED FROM		*/
  /* VIEW: http://api.zeromq.org/zmq_socket.html*/

  void* context = zmq_init (1,1,0);
  void* sock = zmq_socket(context,ZMQ_REP);
  int rc = zmq_connect(sock, endpoint);
  assert(rc == 0);
  return sock;
}
