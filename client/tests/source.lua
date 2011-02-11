require 'zmq'


-- Return sender ID and message body
function recv_from (sock)
  return sock:recv (), sock:recv ()
end

-- Send multipart message into the socket "id | msg"
function send_to (sock, id, msg)
  sock:send (id, zmq.SNDMORE)
  sock:send (msg)
end

function main ()
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
    local req_msg

    -- Wait for request to come in
    req_id, req_msg = recv_from (sock)
    print ("got request: \"" .. req_msg .. "\" from \"" .. req_id .. "\"")

    -- Send out some garbage reply
    send_to (sock, req_id, "hello?")

  end

  -- Close socket
  sock:close ()

  -- Free OMQ context
  ctx:term ()

end

main ()
