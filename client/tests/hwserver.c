/*
 * hello world server (hwserver.c)
 */

#include <stdlib.h>
#include <stdio.h>

#include "easyudp.h"

int
main (void)
{
  struct eu_socket *sock;
  int rc;
  char buf[EU_PACKETLEN] = {0};
  char node[EU_ADDRSTRLEN] = {0};
  char port[EU_PORTSTRLEN] = {0};
  int i;

  sock = eu_socket (EU_PULL);
  if (NULL == sock)
    return 1;

  rc = eu_bind (sock, "127.0.0.1", "12345");
  if (rc)
    return 1;

  for (i = 0; i < 10; i++) {
    ssize_t numbytes;
    
    numbytes = eu_recv (sock, buf, EU_PACKETLEN, 0, node, port);
    if (numbytes <= 0)
      return 1;

    printf ("[%s:%s] sent us \"%s\"\n", node, port, buf);
  }

  eu_close (sock);

  return 0;
}
