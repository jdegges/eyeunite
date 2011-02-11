#ifndef SN_SOURCENODE_H
#define SN_SOURCENODE_

/* TYPES OF MESSAGES THE SOURCE CAN SEND				     */
typedef enum {
  FOLLOW_NODE,
  FEED_NODE,
  DROP_NODE,
  REQ_MOVE,
  REQ_JOIN,
  REQ_EXIT
} message_type ;

typedef struct {
  const char* identity;
  message_type type;
} message_struct;

void*
sn_initzmq (const char* endpoint, const char* pid);

message_struct*
sn_rcvmsg (void* socket);

int
sn_closesocket (void* socket);

int
sn_sendmsg (void* socket, const char* pid, message_type m_type);



#endif
