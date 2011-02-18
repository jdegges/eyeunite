#ifndef EU_EYEUNITE_H
#define EU_EYEUNITE_H

#include <netdb.h>

#define EU_ADDRSTRLEN INET6_ADDRSTRLEN
#define EU_TOKENSTRLEN 224

struct peer_info
{
  char pid[EU_TOKENSTRLEN];
  char addr[EU_ADDRSTRLEN];
  uint16_t port;
  int peerbw;
};

#endif
