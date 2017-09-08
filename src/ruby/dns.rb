require_relative 'init'

require 'socket'
require 'awesome_print'
require 'packetgen'
require 'dnsruby'
include Dnsruby

$dns_resolver = Resolver.new({:nameserver => ["114.114.114.114"]})

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
    dns_req = PacketGen::Header::DNS.new
    dns_req.read(msg)
    domain = dns_req.qd[0].name

    if domain != '.'
      ret = $dns_resolver.query(domain[0...-1])
      ap ret.answer
      ap ret
      @server.send msg, 0, addr[3], addr[1]
    end
  end
end
