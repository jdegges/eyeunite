-- udp-talker.lua

require 'socket'
require 'posix'

local bitrate = 24 + 5 -- measured in kBps (file size / duration)
local packet_size = 500 -- measured in bytes

local listener_addr = "" -- who to send to
local listener_port = "" -- where they want it

local file_handle = io.open ("", "rb") -- what file to send
local sending_socket = socket.udp ()



while true do

  local packet = file_handle:read (packet_size)
  if nil == packet then
    print ("end of stream!")
    return 0
  end

  rv, msg = sending_socket:sendto (packet, listener_addr, listener_port)
  if nil == rv then
    print ("error sending packet to listener: " .. msg)
    return 1
  end

  posix.usleep (packet_size / (bitrate * 0.001024))

end

file_handle:close ()
sending_socket:close ()

return 0
