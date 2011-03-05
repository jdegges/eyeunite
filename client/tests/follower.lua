require 'eu'
require 'socket'

-- Handle the FOLLOW_NODE request
function follow_node (msg)
  --print ("following:", msg.pid, msg.addr, msg.port, msg.bandwidth)
end

-- Handle the FEED_NODE request
function feed_node (msg)
  --print ("feeding \"" .. msg.pid .. "\"")
  downstream_nodes[msg.pid] = msg
end

-- Handle the DROP_NODE request
function drop_node (msg)
  --print ("dropping \"" .. msg.pid .. "\"")
  downstream_nodes[msg.pid] = nil
end

local request_handler = {}
request_handler[eu.FOLLOW_NODE] = follow_node
request_handler[eu.FEED_NODE] = feed_node
request_handler[eu.DROP_NODE] = drop_node

local fn = follower_init ("LUApid\0", "127.0.0.1\0\0\0\0", "1234567\0", "15000000")

fn:connect ("127.0.0.1", "5555")

fn:send_request (eu.REQ_JOIN)

-- create a socket to receive data from upstream provider
local upstream_sock = socket.udp ()
upstream_sock:setsockname ('*', "4567")
upstream_sock:settimeout (1)

-- create a socket to send data downstream
local downstream_sock = socket.udp ()

-- create a table to store all downstream nodes
local downstream_nodes = {}

local current_time = os.time ()

while true do
  local msg_type
  local msg

  if current_time + 10 < os.time () then
    return 0
  end

  -- receive and handle messages from the source
  msg_type, msg = fn:recv_request (zmq.NOBLOCK)
  if nil ~= msg_type then
    current_time = os.time ()
    request_handler[msg_type] (msg)
  end

  -- try to get a udp packet from upstream
  if nil ~= upstream_sock then
    local packet = upstream_sock:receive ()
    if nil ~= packet then
      current_time = os.time ()
      local length = packet:sub (1, 8)
      local data = packet:sub (9, packet:len())

      -- print out the data
      io.write (data)

      -- forward packet to followers
      for index, peer in ipairs (downstream_nodes) do
        local rv = downstream_sock:sendto (packet, peer.addr, peer.port)
        if 1 ~= rv then
          --print ("error sending packet to follower")
          return 1
        end
      end
    end
  end

end
