require 'eu'
require 'socket'
require 'posix'

local my_pid = "LUAsrc"
local my_addr = "131.179.144.43"
local my_port = "12345"
local my_bandwidth = "\209\183\1"

local input_file = io.open ("arctcat.mpg", "rb")
local input_bandwidth = "\209\183\1" -- measured in kBps (file size / duration)
local packet_size = 500 -- measured in bytes


-- Handle the REQ_MOVE request
function req_move (msg)
  --send_to (sock, id, "FOLLOW_NODE", "go go go")
end

-- Handle the REQ_JOIN request
function req_join (msg)
  --send_to (sock, id, "FOLLOW_NODE", "welcome " .. id)
  sn:send_request (msg.pid, eu.FOLLOW_NODE, {pid = my_pid,
                                             addr = my_addr,
                                             port = my_port,
                                             bandwidth = my_bandwidth})
  downstream_nodes[msg.pid] = msg
  print ("got join message from `" .. msg.pid .. "'")
end

-- Handle the REQ_EXIT request
function req_exit (msg)
  --send_to (sock, id, "DROP_NODE", "good bye " .. id)
end

local request_handler = {}
request_handler[eu.REQ_MOVE] = req_move
request_handler[eu.REQ_JOIN] = req_join
request_handler[eu.REQ_EXIT] = req_exit

local sn = source_init (my_pid, my_addr, my_port, my_bandwidth,
                        input_bandwidth)
if nil == sn then
  print ("error initializing the source")
  return 1
end

-- create a socket to send data downstream
local downstream_sock = socket.udp ()

-- create a table to store all downstream nodes
local downstream_nodes = {}

local current_time = os.time ()
local sequence_number = 1
local seqnum_length = 8

while true do

  local from_pid
  local msg_type
  local msg_body

  -- receive and handle messages from follower nodes
  from_pid, msg_type, msg_body = sn:recv_request (zmq.NOBLOCK)
  if nil ~= from_pid then
    if nil ~= msg_body and from_pid ~= msg_body.pid then
      print ("absurd error")
      return 1
    end

    request_handler[msg_type] (msg_body)
  end

  -- read in a chunk of data from input file
  local packet_data = input_file:read (packet_size - seqnum_length)
  if nil == packet_data then
    print ("end of file")
    break
  end

  local packet = string.sub (sequence_number .. string.rep ('\0', seqnum_length),
                             1, seqnum_length) .. packet_data

  -- forward packet to followers
  for index, peer in ipairs (downstream_nodes) do
    local rv = downstream_sock:sendto (packet, peer.addr, peer.port)
    if nil == rv then
      print ("error sending packet to follower")
      return 1
    end
  end

  posix.usleep (3000) -- packet_size / (input_bandwidth * 0.001024))

  sequence_number = sequence_number + 1

end

input_file:close ()
sn:close ()
