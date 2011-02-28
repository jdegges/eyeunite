require 'zmq'

-- API:
-- follower_init (pid, addr, port, bandwidth)
--   Returns a new follower 'object' given its PID, ADDR, PORT, and BANDWIDTH
--
-- follower:connect (addr, port)
--   Given an initialized follower, will try to connect to the source at
--   addr:port. Returns true on success
--
-- follower:send_request (msg_type)
--   Given an initialized and connected follower, will send a message of type
--   msg_type to the source node along with a properly formatted message
--   containing this follower's peer information (pid, addr, port, and bw).
--   Returns true on success. msg_type may be one of eu.REQ_MOVE, eu.REQ_JOIN,
--   or eu.REQ_EXIT.
--
-- follower:recv_request (flags)
--   Given an initialized and connected follower, will try to receive a message
--   from the source. It returns first the message type (can be one of
--   eu.FOLLOW_NODE, eu.FEED_NODE, or eu.DROP_NODE) as well as the message body
--   that will be of the form msg.pid, msg.addr, msg.port, and msg.bandwidth.
--   Flags may be any valid zmq flags. For nonblocking use zmq.NOBLOCK.
--
-- follower:close ()
--   Closes any sockets and frees up any internal state

-- Example usage:
-- > follower = follower_init ("my pid", "my addr", "my udp port", "my bw")
-- > follower:connect ("source address (ip)", "the source's open tcp port")
-- > follower:send_request (eu.REQ_JOIN)
-- > msg_type, msg = follower:recv_request ()
-- > print (msg_type)
-- --> 1
-- > print (msg.pid)
-- --> "the pid of the peer to start following"
-- > print (msg.addr)
-- --> "the address of the peer to start following"
-- > print (msg.port)
-- > print (msg.bandwidth)
-- ...

eu = {
  FOLLOW_NODE = 1,
  FEED_NODE = 2,
  DROP_NODE = 3,
  REQ_MOVE = 4,
  REQ_JOIN = 5,
  REQ_EXIT = 6,

  TOKENSTRLEN = 56,
  ADDRSTRLEN = 46
}

local const = {
  PORTUINTLEN = 2,
  PEERBWINTLEN = 4,
}

const.MSG_PID_START = 1
const.MSG_PID_STOP = const.MSG_PID_START + eu.TOKENSTRLEN
const.MSG_ADDR_START = const.MSG_PID_STOP + 1
const.MSG_ADDR_STOP = const.MSG_ADDR_START + eu.ADDRSTRLEN
const.MSG_PORT_START = const.MSG_ADDR_STOP + 1
const.MSG_PORT_STOP = const.MSG_PORT_START + const.PORTUINTLEN
const.MSG_PEERBW_START = const.MSG_PORT_STOP + 1
const.MSG_PEERBW_STOP = const.MSG_PEERBW_START + const.PEERBWINTLEN

function follower_init (pid, addr, port, bandwidth)
  local follower = {
    pid = pid,
    addr = addr,
    port = port,
    bandwidth = bandwidth,

    connect = follower_connect,
    send_request = follower_send_request,
    recv_request = follower_recv_request
  }

  return follower
end

function follower_connect (follower, addr, port)
  local zmq_ctx = zmq.init (1)
  local sock = zmq_ctx:socket (zmq.XREQ)

  sock:setopt (zmq.IDENTITY, follower.pid)
  sock:connect ("tcp://" .. addr .. ":" .. port)

  follower.zmq_ctx = zmq_ctx
  follower.sock = sock

  return true
end

function follower_send_request (follower, msg_type)
  follower.sock:send (msg_type, zmq.SNDMORE)
  follower.sock:send (follower.pid .. follower.addr .. follower.port
                   .. follower.bandwidth)

  return true
end

function follower_recv_request (follower, flags)
  local msg_type = follower.sock:recv (flags)
  local msg_data = follower.sock:recv (flags)
  local msg = {
    pid = msg_data:sub (const.MSG_PID_START, const.MSG_PID_STOP),
    addr = msg_data:sub (const.MSG_ADDR_START, const.MSG_ADDR_STOP),
    port = msg_data:sub (const.MSG_PORT_START, const.MSG_PORT_STOP),
    bandwidth = msg_data:sub (const.MSG_PEERBW_START, const.MSG_PEERBW_STOP)
  }

  return msg_type, msg
end

function follower_close (follower)
  follower.sock:close ()
  follower.zmq_ctx:term ()

  return true
end
