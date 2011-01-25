/*
 * hello world client (hwclient.c)
 */

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "easyudp.h"


int
main (void)
{
  struct eu_socket *sock;
  int rc;
  int i;

  sock = eu_socket (EU_PUSH);
  if (NULL == sock)
    return 1;

  rc = eu_connect (sock, "127.0.0.1", "12345");
  if (rc)
    return 1;

  for (i = 0; i < 10; i++) {
    ssize_t numbytes;
    numbytes = eu_send (sock, "hello world!", strlen ("hello world!") + 1, 0);
  }

  eu_close (sock);

  return 0;
}
