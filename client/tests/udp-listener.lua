require 'socket'

local udp_timeout = 10 -- seconds
local receiving_socket = socket.udp ()
receiving_socket:setsockname ('*', "") -- put your open port here
receiving_socket:settimeout (1) -- for nonblocking-ness (leave timeout alone)

local file_handle = io.open ("", "wb") -- specify output file here

local current_time = os.time ()

while true do

  if current_time + udp_timeout < os.time () then
    print ("10 seconds of no udp rx, shutting down.")
    break
  end

  local packet = receiving_socket:receive ()
  if nil ~= packet then
    current_time = os.time ()

    file_handle:write (packet)
  end

end

file_handle:close ()
receiving_socket:close ()

return 0
