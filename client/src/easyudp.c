#define _POSIX_SOURCE
#define _BSD_SOURCE

#include "easyudp.h"
#include "debug.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

struct eu_socket
{
  enum eu_sock_type type;
  int fd;
};

struct eu_socket *
eu_socket (enum eu_sock_type type)
{
  struct eu_socket *sock;

  sock = calloc (1, sizeof *sock);
  if (NULL == sock)
    return NULL;

  sock->type = type;
  sock->fd = -1;
  return sock;
}

void
eu_close (struct eu_socket *sock)
{
  if (NULL == sock)
    return;

  close (sock->fd);
  free (sock);
}

int
eu_connect (struct eu_socket *sock, const char *node, const char *port)
{
  struct addrinfo hints, *servinfo, *p;
  int sockfd;
  int rc;

  if (NULL == sock || NULL == node || NULL == port) {
    print_error ("error: invalid arguments");
    return -1;
  }

  if (EU_PUSH != sock->type) {
    print_error ("error: can only bind to EU_PUSH socket");
    return -1;
  }
  
  if (0 < sock->fd) close (sock->fd);

  memset (&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  rc = getaddrinfo (node, port, &hints, &servinfo);
  if (0 != rc) {
    print_error ("getaddrinfo");
    return -1;
  }

  for (p = servinfo; NULL != p; p = p->ai_next) {
    sockfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
    if (-1 == sockfd)
      continue;

    rc = connect (sockfd, p->ai_addr, p->ai_addrlen);
    if (-1 == rc) {
      close (sockfd);
      continue;
    }

    break;
  }

  if (NULL == p) {
    print_error ("failed to connect socket");
    return -1;
  }

  freeaddrinfo (servinfo);

  sock->fd = sockfd;

  return 0;
}

int
eu_bind (struct eu_socket *sock, const char *node, const char *port)
{
  struct addrinfo hints, *servinfo, *p;
  int sockfd;
  int rc;

  if (NULL == sock || NULL == port) {
    print_error ("error: invalid arguments");
    return -1;
  }

  if (EU_PULL != sock->type) {
    print_error ("error: can only bind to EU_PULL socket");
    return -1;
  }

  memset (&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  rc = getaddrinfo (node, port, &hints, &servinfo);
  if (0 != rc) {
    print_error ("getaddrinfo");
    return -1;
  }

  for (p = servinfo; NULL != p; p = p->ai_next) {
    sockfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
    if (-1 == sockfd)
      continue;

    rc = bind (sockfd, p->ai_addr, p->ai_addrlen);
    if (-1 == rc) {
      close (sockfd);
      continue;
    }

    break;
  }

  if (NULL == p) {
    print_error ("failed to bind socket");
    return -1;
  }

  freeaddrinfo (servinfo);

  sock->fd = sockfd;

  return 0;
}

ssize_t
eu_send (struct eu_socket *sock, const void *buf, size_t len, int flags)
{
  ssize_t numbytes;

  if (NULL == sock) {
    print_error ("error: invalid arguments");
    return -1;
  }

  numbytes = send (sock->fd, buf, len, flags);
  if (-1 == numbytes)
    return -2;

  return numbytes;
}

ssize_t
eu_recv (struct eu_socket *sock, void *buf, size_t len, int flags, char *node,
         char *port)
{
  ssize_t numbytes;
  struct sockaddr_storage their_addr;
  socklen_t addr_len = sizeof their_addr;

  if (NULL == sock || NULL == node) {
    print_error ("error: invalid arguments");
    return -1;
  }

  numbytes = recvfrom (sock->fd, buf, len, flags,
                       (struct sockaddr *) &their_addr, &addr_len);
  if (-1 == numbytes) {
    print_error ("recvfrom");
    return -1;
  }

  switch (their_addr.ss_family) {
    case AF_INET:
      if (port) snprintf (port, EU_PORTSTRLEN, "%u",
                          ((struct sockaddr_in *) &their_addr)->sin_port);
      if (node) inet_ntop (AF_INET,
                           &((struct sockaddr_in *) &their_addr)->sin_addr,
                           node, INET_ADDRSTRLEN);
      break;
    case AF_INET6:
      if (port) snprintf (port, EU_PORTSTRLEN, "%u",
                          ((struct sockaddr_in6 *) &their_addr)->sin6_port);
      if (node) inet_ntop (AF_INET6,
                           &((struct sockaddr_in6 *) &their_addr)->sin6_addr,
                           node, INET6_ADDRSTRLEN);
      break;
    default: return -1;
  }

  return numbytes;
}
