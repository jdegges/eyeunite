require 'zmq'


function main ()
  -- Initialize OMQ context
  local ctx = zmq.init (1)

  -- Create a ZMQ_XREQ socket (http://www.zeromq.org/tutorials:xreq-and-xrep)
  local sock = ctx:socket (zmq.XREQ)

  -- Set this followers identity (http://api.zeromq.org/zmq_setsockopt.html)
  sock:setopt (zmq.IDENTITY, "follower!")

  -- Connect to the source node
  sock:connect ("tcp://127.0.0.1:55555")

  -- Send join request to the source
  sock:send ("JOIN plz")

  -- Wait for source reply
  msg = sock:recv ()
  print ("got response: \"" .. msg .. "\"")

  -- Close socket
  sock:close ()

  -- Free OMQ context
  ctx:term ()
end

main ()
