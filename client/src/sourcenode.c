#include "sourcenode.h"

#include <stdlib.h>
#include <zmq.h>
#include <assert.h>
#include <string.h>


// INITIALIZES XREP SOCKET FOR SOURCE TO SEND MESSAGES TO ALL ITS FOLLOWERS 
// params: 								     
// 	const char* endpoint -> interface (i.e. "tcp://*:1555")		     
//	const char* pid -> node's identity				     
void*
sn_initzmq (const char* endpoint, const char* pid) {

  // INITIALIZE CONTEXT AND CREATE SOCKET
  void* context = zmq_init (1);
  void* sock = zmq_socket(context,ZMQ_XREP);
  int rc = zmq_bind(sock, endpoint);
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
sn_closesocket (void* socket) {
  return zmq_close(socket);
}


message_struct*
sn_rcvmsg (void* socket) {
  // NEEDED TO RETRIEVE ALL MESSAGE PARTS
  int64_t more;
  size_t more_size = sizeof (more);

  // GET THE FIRST PART OF THE MESSAGE, SHOULD BE IDENTITY
  zmq_msg_t req_id;
  int rc = zmq_msg_init (&req_id);
  assert (rc == 0);
  rc = zmq_recv (socket, &req_id, ZMQ_NOBLOCK);
  
  // IF NO MESSAGES RETURN NULL
  if (rc == -1) {
	assert (errno != EAGAIN);
	return NULL;
  }

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || !more);

  // GET THE DELIMITER
  zmq_msg_t delim;
  rc = zmq_msg_init (&delim);
  assert (rc == 0);
  rc = zmq_recv (socket, &delim, ZMQ_NOBLOCK);
  assert (rc == -1);
  zmq_msg_close(&delim);

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || !more);

  // GET THE REQUEST TYPE
  zmq_msg_t req;
  rc = zmq_msg_init (&req);
  assert (rc == 0);
  rc = zmq_recv (socket, &req, ZMQ_NOBLOCK);
  assert (rc == -1);

  // STORE IN A MESSAGE_STRUCT
  message_struct* req_msg = malloc (sizeof(message_struct));
  assert (req_msg);
  void* id_data = zmq_msg_data (&req_id);
  size_t id_size = zmq_msg_size (&req_id);
  memcpy (&(req_msg->identity), id_data, id_size);
  zmq_msg_close (&req_id);

  void* req_data = zmq_msg_data (&req);
  size_t req_size = zmq_msg_size (&req);
  memcpy (&(req_msg->type), req_data, req_size);
  zmq_msg_close (&req);

  // GET DATA (IF JOIN)
  if ( req_msg->type == REQ_JOIN ) {
	
	// MAKE SURE THERE'S MORE
	rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
	assert (rc == -1 || !more);

	zmq_msg_t join;
	rc = zmq_msg_init (&join);
	assert (rc = 0);
	rc = zmq_recv (socket, &join, ZMQ_NOBLOCK);
	assert (rc == -1);

	void* join_data = zmq_msg_data (&join);
	size_t join_size = zmq_msg_size (&join);
	memcpy (&(req_msg->node_params), join_data, join_size);
	zmq_msg_close (&join);
  }

  // MAKE SURE THERE'S NO MORE
  rc = zmq_getsockopt( socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == -1 || more);

  return req_msg;
}




// USED TO SEND A MESSAGE TO FOLLOWER NODE
// params:
// 	void* socket -> pointer to socket sending from
//	const char* pid -> destination identity
int
sn_sendmsg (void* socket, const char* pid, const char* type, struct tnode* params) {
  int rc = 0;

  // CREATE IDENTITY MESSAGE
  void *dest = malloc(strlen(pid));
  assert (dest);
  memcpy (dest, pid, strlen(pid));
  zmq_msg_t identity_message;
  rc += zmq_msg_init_data (&identity_message, dest, strlen(pid), NULL, NULL);

  // CREATE MESSAGE TYPE
  void *mtype = malloc(MAX_MESSAGE_TYPE);
  assert (mtype);
  memcpy (mtype, &type, MAX_MESSAGE_TYPE);
  zmq_msg_t mtype_message;
  rc += zmq_msg_init_data 
		(&mtype_message, mtype, MAX_MESSAGE_TYPE, NULL, NULL);

  // CREATE NODE PARAMETERS
  void *node_param = malloc(sizeof(struct tnode));
  assert (node_param);
  memcpy (node_param, params, sizeof(struct tnode));
  zmq_msg_t node_message;
  rc += zmq_msg_init_data (&node_message, node_param, sizeof(struct tnode), NULL, NULL);

  if (rc != 0) {
	return -1;
  }

  // SEND MULTIPLE MESSAGES OUT SOCKET
  rc += zmq_send (socket, &identity_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, NULL, ZMQ_SNDMORE);
  rc += zmq_send (socket, &mtype_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &node_message, 0);

  if (rc != 0) {
	return -1;
  }
  else {
	return 0;
  }
}


