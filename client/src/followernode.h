#ifndef FN_FOLLOWERNODE_H
#define FN_FOLLOWERNODE_H

#include <sourcenode.h>

void*
fn_initzmq (const char* pid, const char* connect_to);

int
fn_closesocket (void* socket);

message_struct*
fn_rcvmsg (void* socket);

int
fn_sendmsg (void* socket, message_type type, struct peer_info* params);

#endif
