#include "debug.h"
#include "followernode.h"

#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <string.h>

// INITIALIZES XREQ SOCKET FOR FOLLOWER TO SEND MESSAGES TO ITS SOURCE
// params: 								     
// 	const char* endpoint -> interface (i.e. "tcp://*:1555")		     
//	const char* pid -> node's identity				     
void*
fn_initzmq (const char* pid, const char* connect_to) {

  // INITIALIZE CONTEXT AND CREATE SOCKET
  void* context = zmq_init (1); // XXX TODO: save this pointer and call zmq_term (context) at exit
  void* sock = zmq_socket(context,ZMQ_XREQ);

  // SET THE IDENTITY FOR LATER USE
  int rc = zmq_setsockopt (sock, ZMQ_IDENTITY, pid, strlen(pid)+1);
  assert(rc == 0);

  // CONNECT TO THE SOURCE
  rc = zmq_connect(sock, connect_to);
  assert(rc == 0);

  return sock;
}

// DESTROYS SOCKET AND KILLS ALL ACTIVE	CONNECTIONS
// params:
//	void* socket -> socket to be closed
int
fn_closesocket (void* socket) {
  return zmq_close(socket);
}

message_struct*
fn_rcvmsg (void* socket) {
  // NEEDED TO RETRIEVE ALL MESSAGE PARTS
  int64_t more;
  size_t more_size = sizeof (more);

  // GET THE MESSAGE TYPE
  zmq_msg_t cmd_type;
  int rc = zmq_msg_init (&cmd_type);
  assert (rc == 0);
  rc = zmq_recv (socket, &cmd_type, ZMQ_NOBLOCK);

  // IF NO MESSAGES RETURN NULL
  if (rc == -1) {
	assert (errno == EAGAIN);
	return NULL;
  }

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == 0 && more);

  // GET INFO ON NODE TO FOLLOW OR FEED
  zmq_msg_t cmd_info;
  rc = zmq_msg_init (&cmd_info);
  assert (rc == 0);
  rc = zmq_recv (socket, &cmd_info, ZMQ_NOBLOCK);
  assert (rc == 0);

  // STORE IN A MESSAGE_STRUCT
  message_struct* cmd_msg = malloc (sizeof(message_struct));
  assert (cmd_msg);
  

  void* cmd_type_data = zmq_msg_data (&cmd_type);
  size_t cmd_type_size = zmq_msg_size (&cmd_type); // XXX TODO: check size!
  memcpy (&(cmd_msg->type), cmd_type_data, cmd_type_size);
  zmq_msg_close (&cmd_type);

  void* cmd_info_data = zmq_msg_data (&cmd_info);
  size_t cmd_info_size = zmq_msg_size (&cmd_info); // XXX TODO: check size!
  memcpy (&(cmd_msg->node_params), cmd_info_data, cmd_info_size);
  zmq_msg_close (&cmd_info);


  // MAKE SURE THERE'S NO MORE
  rc = zmq_getsockopt( socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == 0 && !more);

  return cmd_msg;
}


// USED TO SEND A MESSAGE TO SOURCE NODE
// params:
// 	void* socket -> pointer to socket sending from
//	const char* pid -> destination identity
int
fn_sendmsg (void* socket, message_type type, struct peer_info* params) {
  int rc = 0;

  // CREATE MESSAGE TYPE
  zmq_msg_t mtype_message;
  rc += zmq_msg_init_size (&mtype_message, sizeof(message_type));
  memcpy (zmq_msg_data (&mtype_message), &type, sizeof(message_type));

  // CREATE NODE PARAMETERS
  zmq_msg_t node_message;
  rc += zmq_msg_init_size (&node_message,  sizeof(struct peer_info));
  memcpy (zmq_msg_data (&node_message), params, sizeof(struct peer_info));

  if (rc != 0) {
    print_error ("");
    return -1;
  }

  // SEND MULTIPLE MESSAGES OUT SOCKET
  rc += zmq_send (socket, &mtype_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &node_message, 0);

  if (rc != 0) {
    print_error ("");
    return -1;
  }

  return 0;
}
