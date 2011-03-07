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
  FOLLOW_NODE = "\1\0\0\0",
  FEED_NODE = "\2\0\0\0",
  DROP_NODE = "\3\0\0\0",
  REQ_MOVE = "\4\0\0\0",
  REQ_JOIN = "\5\0\0\0",
  REQ_EXIT = "\6\0\0\0",

  TOKENSTRLEN = 7,
  ADDRSTRLEN = 16,
  PORTSTRLEN = 7
}

local const = {
  PEERBWINTLEN = 4,
}

const.MSG_PID_START = 1
const.MSG_PID_STOP = const.MSG_PID_START + eu.TOKENSTRLEN
const.MSG_ADDR_START = const.MSG_PID_STOP
const.MSG_ADDR_STOP = const.MSG_ADDR_START + eu.ADDRSTRLEN
const.MSG_PORT_START = const.MSG_ADDR_STOP
const.MSG_PORT_STOP = const.MSG_PORT_START + eu.PORTSTRLEN
const.MSG_PEERBW_START = const.MSG_PORT_STOP
const.MSG_PEERBW_STOP = const.MSG_PEERBW_START + const.PEERBWINTLEN

function follower_init (pid, addr, port, bandwidth)
  local null_string = string.rep ('\0', 32)

  if eu.TOKENSTRLEN < pid:len () or eu.ADDRSTRLEN < addr:len () or
     eu.PORTSTRLEN < port:len () or 6 < bandwidth:len () then
    return nil
  end

  local follower = {
    pid = string.sub (pid .. null_string, 1, eu.TOKENSTRLEN),
    addr = string.sub (addr .. null_string, 1, eu.ADDRSTRLEN),
    port = string.sub (port .. null_string, 1, eu.PORTSTRLEN),
    bandwidth = string.sub (bandwidth .. null_string, 1, 6),

    connect = follower_connect,
    send_request = follower_send_request,
    recv_request = follower_recv_request,
    close = follower_close
  }

  return follower
end

function source_init (pid, addr, port, bandwidth, stream_bandwidth)
  local null_string = string.rep ('\0', 32)

  if eu.TOKENSTRLEN < pid:len () or eu.ADDRSTRLEN < addr:len () or
     eu.PORTSTRLEN < port:len () or 6 < bandwidth:len () or
     6 < stream_bandwidth:len () then
    return nil
  end

  local zmq_ctx = zmq.init (1)
  local sock = zmq_ctx:socket (zmq.XREP)
  sock:setopt (zmq.IDENTITY, pid)
  sock:bind ("tcp://*:" .. port)

  local source = {
    pid = string.sub (pid .. null_string, 1, eu.TOKENSTRLEN),
    addr = string.sub (addr .. null_string, 1, eu.ADDRSTRLEN),
    port = string.sub (port .. null_string, 1, eu.PORTSTRLEN),
    bandwidth = string.sub (bandwidth .. null_string, 1, 6),
    stream_bandwidth = string.sub (stream_bandwidth .. null_string, 1, 6),

    zmq_ctx = zmq_ctx,
    sock = sock,

    send_request = source_send_request,
    recv_request = source_recv_request,
    close = source_close
  }

  return source
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

  local msg = follower.pid .. follower.addr .. follower.port ..
              follower.bandwidth
  follower.sock:send (msg)

  return true
end

function source_send_request (source, msg_type, msg_data)
  source.sock:send (msg_data.pid, zmq.SNDMORE)
  source.sock:send (msg_type, zmq.SNDMORE)

  local msg = msg_data.pid .. msg_data.addr .. msg_data.port ..
              msg_data.bandwidth
  source.sock:send (msg)

  return true
end

function follower_recv_request (follower, flags)
  local msg_type = follower.sock:recv (flags)
  if nil == msg_type then
    return nil
  end

  local msg_data = follower.sock:recv (flags)
  if nil == msg_data then
    print ("absurd error 1")
    return nil
  end

  local msg = {
    pid = msg_data:sub (const.MSG_PID_START, const.MSG_PID_STOP),
    addr = msg_data:sub (const.MSG_ADDR_START, const.MSG_ADDR_STOP),
    port = msg_data:sub (const.MSG_PORT_START, const.MSG_PORT_STOP),
    bandwidth = msg_data:sub (const.MSG_PEERBW_START, const.MSG_PEERBW_STOP)
  }

  return msg_type, msg
end

function source_recv_request (source, flags)
  local from_pid = source.sock:recv (flags)
  if nil == from_pid then
    return nil
  end

  local msg_type = source.sock:recv (flags)
  if nil == msg_type then
    print ("absurd error 2")
    return nil
  end

  local msg_body = nil
  if eu.REQ_JOIN == msg_type then
    local msg_data = source.sock:recv (flags)
    if nil == msg_data then
      print ("absurd error 3")
      return nil
    end

    local msg_body = {
      pid = msg_data:sub (const.MSG_PID_START, const.MSG_PID_STOP),
      addr = msg_data:sub (const.MSG_ADDR_START, const.MSG_ADDR_STOP),
      port = msg_data:sub (const.MSG_PORT_START, const.MSG_PORT_STOP),
      bandwidth = msg_data:sub (const.MSG_PEERBW_START, const.MSG_PEERBW_STOP)
    }

    print (msg_body.pid, msg_body.pid:len())
    print (msg_body.addr, msg_body.addr:len())
    print (msg_body.port, msg_body.port:len())
    print (msg_body.bandwidth:reverse(), msg_body.bandwidth:len())

    if string.find (from_pid, msg_body.pid) ~= 1 then
      print ("absurd error 4", from_pid, msg_body.pid)
      print ("from_pid: " .. from_pid:len ())
      print ("pid     : " .. msg_body.pid:len ())
      return nil
    end
  end

  return from_pid, msg_type, msg_body
end

function follower_close (follower)
  follower.sock:close ()
  follower.zmq_ctx:term ()

  return true
end

function source_close (source)
  source.sock:close ()
  source.zmq_ctx:term ()

  return true
end
