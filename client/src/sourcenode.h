#ifndef SN_SOURCENODE_H
#define SN_SOURCENODE_H

#include <tree.h>

#define MAX_MESSAGE_TYPE 11

// TYPES OF MESSAGES THE SOURCE CAN SEND
#define FOLLOW_NODE "FOLLOW_NODE"
#define FEED_NODE "FEED_NODE"
#define DROP_NODE "DROP_NODE"
#define REQ_MOVE "REQ_MOVE"
#define REQ_JOIN "REQ_JOIN"
#define REQ_EXIT "REQ_EXIT"

typedef struct {
  const char* identity;
  const char type[MAX_MESSAGE_TYPE];
  tnode node_params;
} message_struct;

void*
sn_initzmq (const char* endpoint, const char* pid);

int
sn_closesocket (void* socket);

message_struct*
sn_rcvmsg (void* socket);

int
sn_sendmsg (void* socket, const char* pid, const char* m_type, tnode* params);



#endif
