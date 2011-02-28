#ifndef FN_FOLLOWERNODE_H
#define FN_FOLLOWERNODE_H

#include "sourcenode.h"

#define ADD_DOWNSTREAM_PEER "ADD_DOWNSTREAM_PEER"
#define DROP_DOWNSTREAM_PEER "DROP_DOWNSTREAM_PEER"
#define CHANGE_UPSTREAM_PEER "CHANGE_UPSTREAM_PEER"

#define MAX_PEERS 100

void*
fn_initzmq (const char* pid, const char* connect_to);

int
fn_closesocket (void* socket);

message_struct*
fn_rcvmsg (void* socket);

int
fn_sendmsg (void* socket, message_type type, struct peer_info* params);

#endif
