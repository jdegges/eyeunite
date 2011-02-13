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
fn_initzmq (const char* endpoint, const char* pid, const char* connect_to) {

  // INITIALIZE CONTEXT AND CREATE SOCKET
  void* context = zmq_init (1);
  void* sock = zmq_socket(context,ZMQ_XREQ);
  int rc = zmq_bind(sock, endpoint);
  assert(rc == 0);
  rc = zmq_connect(sock, connect_to);
  assert(rc == 0);

  // SET THE IDENTITY FOR LATER USE
  rc = zmq_setsockopt (sock, ZMQ_IDENTITY, pid, strlen(pid));
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

  // GET THE FIRST PART OF THE MESSAGE, SHOULD BE IDENTITY
  zmq_msg_t cmd_id;
  int rc = zmq_msg_init (&cmd_id);
  assert (rc == 0);
  rc = zmq_recv (socket, &cmd_id, ZMQ_NOBLOCK);
  
  // IF NO MESSAGES RETURN NULL
  if (rc == -1) {
	assert (errno != EAGAIN);
	return;
  }

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || !more);

  // GET THE DELIMITER
  zmq_msg_t delim;
  rc = zmq_msg_init (&delim);
  assert (rc == 0);
  rc = zmq_recv (socket, &delim, ZMQ_NOBLOCK);
  assert (rc == -1 || &delim == NULL);
  zmq_msg_close(&delim);

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || !more);

  // GET THE MESSAGE TYPE
  zmq_msg_t cmd_type;
  rc = zmq_msg_init (&cmd_type);
  assert (rc == 0);
  rc = zmq_recv (socket, &cmd_type, ZMQ_NOBLOCK);
  assert (rc == -1);

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || !more);

  // GET INFO ON NODE TO FOLLOW OR FEED
  zmq_msg_t cmd_info;
  rc = zmq_msg_init (&cmd_info);
  assert (rc = 0);
  rc = zmq_recv (socket, &cmd_info, ZMQ_NOBLOCK);
  assert (rc == -1);

  // STORE IN A MESSAGE_STRUCT
  message_struct* cmd_msg = malloc (sizeof(message_struct));
  assert (cmd_msg);
  void* id_data = zmq_msg_data (&cmd_id);
  size_t id_size = zmq_msg_size (&cmd_id);
  memcpy (&(cmd_msg->identity), id_data, id_size);
  zmq_msg_close (&cmd_id);

  void* cmd_type_data = zmq_msg_data (&cmd_type);
  size_t cmd_type_size = zmq_msg_size (&cmd_type);
  memcpy (&(cmd_msg->type), cmd_type_data, cmd_type_size);
  zmq_msg_close (&cmd_type);

  void* cmd_info_data = zmq_msg_data (&cmd_info);
  size_t cmd_info_size = zmq_msg_size (&cmd_info);
  memcpy (&(cmd_msg->node_params), cmd_info_data, cmd_info_size);
  zmq_msg_close (&cmd_info);


  // MAKE SURE THERE'S NO MORE
  rc = zmq_getsockopt( socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || more);

  return cmd_msg;
}

// USED TO SEND A MESSAGE TO SOURCE NODE
// params:
// 	void* socket -> pointer to socket sending from
//	const char* pid -> destination identity
int
fn_sendmsg (void* socket, const char* pid, const char* type, tnode* params) {
  int rc = 0;

  // CREATE IDENTITY MESSAGE
  void *dest = malloc(strlen(pid));
  assert (dest);
  memcpy (dest, pid, strlen(pid));
  zmq_msg_t identity_message;
  rc += zmq_msg_init_data (&identity_message, dest, strlen(pid), NULL, NULL);

  // CREATE DELIMITER
  zmq_msg_t delim_message;
  rc += zmq_msg_init_data (&delim_message, NULL, 0, NULL, NULL);
 

  // CREATE MESSAGE TYPE
  void *mtype = malloc(MAX_MESSAGE_TYPE);
  assert (mtype);
  memcpy (mtype, &type, MAX_MESSAGE_TYPE);
  zmq_msg_t mtype_message;
  rc += zmq_msg_init_data 
		(&mtype_message, mtype, MAX_MESSAGE_TYPE, NULL, NULL);

  // CREATE NODE PARAMETERS
  void *node_param = malloc(sizeof(tnode));
  assert (node_param);
  memcpy (node_param, params, sizeof(tnode));
  zmq_msg_t node_message;
  rc += zmq_msg_init_data (&node_message, node_param, sizeof(tnode), NULL, NULL);

  if (rc != 0) {
	return -1;
  }

  // SEND MULTIPLE MESSAGES OUT SOCKET
  rc += zmq_send (socket, &identity_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &delim_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &mtype_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &node_message, 0);

  if (rc != 0) {
	return -1;
  }
  else {
	return 0;
  }
return 0;
}
