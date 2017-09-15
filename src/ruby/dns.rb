require_relative 'init'

require 'socket'
require 'awesome_print'

class DNSServer
  def initialize(host, port)
    @server = UDPSocket.new
    @server.bind(host, port)
    @ev_io = Rbev::IO.new()
    @ev_io.io_register(IO.try_convert(@server), :r, proc { read })
  end

  def run
    @ev_io.io_start
  end

  def read
    msg, addr = @server.recvfrom_nonblock(1460)
    @server.send msg, 0, addr[3], addr[1]
  end
end
