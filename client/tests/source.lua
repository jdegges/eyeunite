require 'zmq'


-- Return sender ID and message body
function recv_from (sock)
  return sock:recv (), sock:recv (), sock:recv ()
end

-- Send multipart message into the socket "id | msg"
function send_to (sock, id, kind, msg)
  sock:send (id, zmq.SNDMORE)
  sock:send (kind, zmq.SNDMORE)
  sock:send (msg)
end

-- Handle the REQ_MOVE request
function req_move (sock, id, msg)
  send_to (sock, id, "FOLLOW_NODE", "go go go")
end

-- Handle the REQ_JOIN request
function req_join (sock, id, msg)
  send_to (sock, id, "FOLLOW_NODE", "welcome " .. id)
end

-- Handle the REQ_EXIT request
function req_exit (sock, id, msg)
  send_to (sock, id, "DROP_NODE", "good bye " .. id)
end

local request_handler = {
  REQ_MOVE = req_move,
  REQ_JOIN = req_join,
  REQ_EXIT = req_exit
}

function source ()
  -- Initialize OMQ context
  local ctx = zmq.init (1)

  -- Create a ZMQ_XREP socket (http://www.zeromq.org/tutorials:xreq-and-xrep)
  local sock = ctx:socket (zmq.XREP)

  -- Set this followers identity (http://api.zeromq.org/zmq_setsockopt.html)
  sock:setopt (zmq.IDENTITY, "source!")

  -- Start listening for requests on port 55555 (thats five 5s)
  sock:bind ("tcp://*:55555")

  while true do

    local req_id
    local req_type
    local req_msg

    -- Wait for request to come in
    req_id, req_type, req_msg = recv_from (sock)
    print ("got \"" .. req_type .. "\" request: \"" .. req_msg ..
           "\" from \"" .. req_id .. "\"")

    -- Handle the request
    request_handler[req_type] (sock, req_id, req_msg)

  end

  -- Close socket
  sock:close ()

  -- Free OMQ context
  ctx:term ()

end

source ()
