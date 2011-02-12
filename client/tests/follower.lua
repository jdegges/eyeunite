require 'zmq'

function sleep(n)
  local t0 = os.clock()
  while os.clock() - t0 <= n do end
end

-- Send a multipart message to the source like "type|msg"
function send_request (sock, msg_type, msg)
  sock:send (msg_type, zmq.SNDMORE)
  sock:send (msg)
end

-- Receive a multipart message from the source like "type|msg"
function recv_request (sock, flags)
  return sock:recv (flags), sock:recv (flags)
end

-- Handle the FOLLOW_NODE request
function follow_node (msg)
  print ("following \"" .. msg .. "\"")
end

-- Handle the FEED_NODE request
function feed_node (msg)
  print ("feeding \"" .. msg .. "\"")
end

-- Handle the DROP_NODE request
function drop_node (msg)
  print ("dropping \"" .. msg .. "\"")
end

local request_handler = {
  FOLLOW_NODE = follow_node,
  FEED_NODE = feed_node,
  DROP_NODE = drop_node
}

function follow ()
  -- Initialize OMQ context
  local ctx = zmq.init (1)

  -- Create a ZMQ_XREQ socket (http://www.zeromq.org/tutorials:xreq-and-xrep)
  local sock = ctx:socket (zmq.XREQ)

  -- Set this followers identity (http://api.zeromq.org/zmq_setsockopt.html)
  sock:setopt (zmq.IDENTITY, "follower!")

  -- Connect to the source node
  sock:connect ("tcp://127.0.0.1:55555")

  -- Send join request to the source
  send_request (sock, "REQ_JOIN", "plzzzz")

  while true do

    local req_type
    local req_msg

    -- Non-blocking wait for source message
    req_type, req_msg = recv_request (sock, zmq.NOBLOCK)
    if req_type ~= nil and req_msg ~= nil then
      request_handler[req_type] (req_msg)
    end

    local rv = math.random ()
    if rv < 0.85 then
      -- Do nothing with 85% chance
      sleep (1)
    elseif rv < .99 then
      -- Send move request with 14% chance
      send_request (sock, "REQ_MOVE", "kthx")
    else
      -- Exit with 1% chance
      send_request (sock, "REQ_EXIT", "bye")
      break
    end

  end

  req_type, req_msg = recv_request (sock, 0)
  request_handler[req_type] (req_msg)

  -- Close socket
  sock:close ()

  -- Free OMQ context
  ctx:term ()
end

math.randomseed (os.time ())

follow ()
