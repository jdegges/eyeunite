#ifndef EU_EYEUNITE_H
#define EU_EYEUNITE_H

#include <netdb.h>
#include <easyudp.h>

#define EU_ADDRSTRLEN INET6_ADDRSTRLEN
#define EU_TOKENSTRLEN 224
#define APP_ENGINE "http://eyeunite.appspot.com"

struct peer_info
{
  char pid[EU_TOKENSTRLEN];
  char addr[EU_ADDRSTRLEN];
  uint16_t port;
  int peerbw;
};

struct data_pack
{
  uint64_t seqnum;
  char data[EU_PACKETLEN - sizeof (uint64_t)];
};

#endif
