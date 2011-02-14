#ifndef SN_SOURCENODE_H
#define SN_SOURCENODE_H

#include <tree.h>

typedef enum {
  FOLLOW_NODE = 1,
  FEED_NODE = 2,
  DROP_NODE = 3,
  REQ_MOVE = 4,
  REQ_JOIN = 5,
  REQ_EXIT = 6
} message_type;

typedef struct {
  const char* identity;
  message_type type;
  struct tnode node_params;
} message_struct;

void*
sn_initzmq (const char* endpoint, const char* pid);

int
sn_closesocket (void* socket);

message_struct*
sn_rcvmsg (void* socket);

int
sn_sendmsg (void* socket, const char* pid, message_type m_type, struct tnode* params);

char* 
sn_mtype_to_string (message_type type);



#endif
