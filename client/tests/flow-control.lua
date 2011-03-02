-- example usage:
-- cat video.mpg | lua flow-control.lua | namedpipe.fifo &
-- vlc namedpipe.fifo

local rate = 0.1          -- will corrupt packets with this rate
local drop = true         -- if true, packets will be dropped at the specified
                          --   rate
local packet_size = 65535 -- packet size...

math.randomseed (os.time ())

while true do

  local packet = io.stdin:read (packet_size)

  if nil == packet then
    break
  end

  local rv = math.random ()
  if true == drop and rv < rate then
    -- drop
  else
    io.stdout:write (packet)
  end

end
