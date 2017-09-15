require_relative 'init'

require 'socket'
require 'awesome_print'

require_relative 'http_client'

class HTTPProxyServer
  attr_accessor :selectables

  def initialize(host, port)
    @selectables = {}

    @server = TCPServer.new(host, port)
    @ev_io = Rbev::IO.new()
    @ev_io.io_register(IO.try_convert(@server), :r, proc {accept})
  end

  def run
    @ev_io.io_start
  end

  def accept
    client = @server.accept_nonblock
    _, port, host = client.peeraddr
    ap "#{host}:#{port} connected"
    @selectables[client] = HTTPClient.new(client, self)
  end
end
