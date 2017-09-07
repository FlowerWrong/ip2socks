require 'socket'

host = '119.23.10.5'
port = 80

s = TCPSocket.open host, port
s.puts "GET / HTTP/1.1\r\n"
s.puts "\r\n"
io = IO.try_convert(s)

m = Rbev::IO.new()

io_cb = proc {
  if s.eof?
    s.close
    m.io_stop
  else
    p s.read(1460)
  end
}

timer_cb = proc {
  p "timer ------------------------------"
  m.timer_stop
}

m.timer_register(15.0, 0, timer_cb)
m.timer_start

m.io_register(io, :r, io_cb)
m.io_start
