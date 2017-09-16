require_relative 'init'

require 'socket'
require 'logger'

require_relative 'dns_client'

class DNSServer
  UDP_TRUNCATION_SIZE = 512

  def initialize(host, port)
    @server = UDPSocket.new
    @server.bind(host, port)
    @ev_io = Rbev::IO.new()
    @ev_io.io_register(IO.try_convert(@server), :r, proc {read})

    @logger = Logger.new(STDOUT)
    @logger.level = Logger::DEBUG

    @counter = 0
  end

  def run
    @ev_io.io_start
  end

  def read
    @counter += 1
    DNSClient.new(@server, @logger).handler
  end
end
