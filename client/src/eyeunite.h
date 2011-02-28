#ifndef EU_EYEUNITE_H
#define EU_EYEUNITE_H

#include <netdb.h>
#include "easyudp.h"

#define EU_ADDRSTRLEN INET6_ADDRSTRLEN
#define EU_TOKENSTRLEN 7
#define FILELOC "log.txt"
#define APP_ENGINE "http://131.179.144.41:8080"

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
