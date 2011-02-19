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
  void* context = zmq_init (1); // XXX TODO: save this pointer and call zmq_term (context) at exit
  void* sock = zmq_socket(context,ZMQ_XREP);

  // SET THE IDENTITY FOR LATER USE
  int rc = zmq_setsockopt (sock, ZMQ_IDENTITY, pid, strlen(pid));
  assert(rc == 0);

  // BIND TO SOCKET
  rc = zmq_bind(sock, endpoint);
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
    assert (errno == EAGAIN);
    return NULL;
  }

  // MAKE SURE THERE'S MORE
  rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
  assert (rc == 0 && more);

  // GET THE REQUEST TYPE
  zmq_msg_t req;
  rc = zmq_msg_init (&req);
  assert (rc == 0);
  rc = zmq_recv (socket, &req, ZMQ_NOBLOCK);
  assert (rc == 0);

  // STORE IN A MESSAGE_STRUCT
  message_struct* req_msg = malloc (sizeof(message_struct));
  assert (req_msg);
  void* id_data = zmq_msg_data (&req_id);
  size_t id_size = zmq_msg_size (&req_id);
  req_msg->identity = malloc (id_size);
  assert (req_msg->identity);
  memcpy (req_msg->identity, id_data, id_size);
  zmq_msg_close (&req_id);

  void* req_data = zmq_msg_data (&req);
  size_t req_size = zmq_msg_size (&req);
  memcpy (&(req_msg->type), req_data, req_size);
  zmq_msg_close (&req);

  // GET DATA (IF JOIN)
  if ( req_msg->type == REQ_JOIN ) {
	
    // MAKE SURE THERE'S MORE
    rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
    assert (rc == 0 && more);

    zmq_msg_t join;
    rc = zmq_msg_init (&join);
    assert (rc == 0);
    rc = zmq_recv (socket, &join, ZMQ_NOBLOCK);
    assert (rc == 0);

    void* join_data = zmq_msg_data (&join);
    size_t join_size = zmq_msg_size (&join);
    // XXX TODO: check that join_size is what we expect!
    assert(join_size == sizeof(struct peer_info));
    memcpy (&(req_msg->node_params), join_data, join_size);
    zmq_msg_close (&join);
  }

  // MAKE SURE THERE'S NO MORE XXX TODO FIXME: when the above if statement is fixed this should be re-enabled
//  rc = zmq_getsockopt( socket, ZMQ_RCVMORE, &more, &more_size);
//  assert (rc == 0 && !more);

  return req_msg;
}




// USED TO SEND A MESSAGE TO FOLLOWER NODE
// params:
// 	void* socket -> pointer to socket sending from
//	const char* pid -> destination identity
int
sn_sendmsg (void* socket, const char* pid, message_type type, struct peer_info* params) {
  int rc = 0;

  // CREATE IDENTITY MESSAGE
  zmq_msg_t identity_message;
  rc += zmq_msg_init_size (&identity_message, strlen(pid)+1);
  memcpy (zmq_msg_data (&identity_message), pid, strlen(pid)+1);

  // CREATE MESSAGE TYPE
  zmq_msg_t mtype_message;
  rc += zmq_msg_init_size (&mtype_message, sizeof(message_type));
  memcpy (zmq_msg_data (&mtype_message), &type, sizeof(message_type));

  // CREATE NODE PARAMETERS
  zmq_msg_t node_message;
  rc += zmq_msg_init_size (&node_message, sizeof(struct peer_info));
  memcpy (zmq_msg_data (&node_message), params, sizeof(struct peer_info));

  if (rc != 0) {
	return -1;
  }

  // SEND MULTIPLE MESSAGES OUT SOCKET
  rc += zmq_send (socket, &identity_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &mtype_message, ZMQ_SNDMORE);
  rc += zmq_send (socket, &node_message, 0);

  if (rc != 0) {
	return -1;
  }
	return 0;
}

// USED TO CONVERT ENUM VALUE TO STRING
char* 
sn_mtype_to_string (message_type type) {
  switch (type) {
    case FOLLOW_NODE:
      return "FOLLOW_NODE";
    case FEED_NODE:
      return "FEED_NODE";
    case DROP_NODE:
      return "DROP_NODE";
    case REQ_MOVE:
      return "REQ_MOVE";
    case REQ_JOIN:
      return "REQ_JOIN";
    case REQ_EXIT:
      return "REQ_EXIT";
    default:
      return "NO_ENUM_MATCH";
    }
}
