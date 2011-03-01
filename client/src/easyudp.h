#ifndef EU_EASYUDP_H
#define EU_EASYUDP_H

#include "eyeunite.h"

#include <stdlib.h>

enum eu_sock_type
{
  EU_PUSH,
  EU_PULL
};

struct eu_socket;


struct eu_socket *
eu_socket (enum eu_sock_type type);

void
eu_close (struct eu_socket *sock);

int
eu_connect (struct eu_socket *sock, const char *node, const char *port);

int
eu_bind (struct eu_socket *sock, const char *node, const char *port);

/* eu_send:
 *  `sock' must have been connected to an address and port with `eu_connect()'
 *  `buf' points to allocatd memory `len' bytes long (maximum of EU_PACKETLEN
 *        bytes supported)
 *  `flags' see man 2 send
 */
ssize_t
eu_send (struct eu_socket *sock, const void *buf, size_t len, int flags);

/* eu_recv:
 *  `sock' must have been bound to an address and port with `eu_bind()'
 *  `buf' must point to allocated memory `len' bytes long (EU_PACKETLEN bytes
 *        recommended)
 *  `flags' see man 2 recv
 *  `node' must point to a buffer EU_ADDRSTRLEN bytes long (enough to hold the
 *         senders address)
 *  `port' must point to a buffer EU_PORTSTRLEN bytes long
 */
ssize_t
eu_recv (struct eu_socket *sock, void *buf, size_t len, int flags, char *node,
         char *port);

#endif
