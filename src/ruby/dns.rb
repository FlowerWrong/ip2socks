require 'socket'

class DNSServer
  def initialize(host, port)
    @selectables = {}

    @server = TCPServer.new(host, port)
    @selectables[@server] = Rbev::IO.new()
    @selectables[@server].io_register(IO.try_convert(@server), :r, proc { accept })
  end

  def run
    @selectables[@server].io_start
  end

  def accept
    client = @server.accept_nonblock
    _, port, host = client.peeraddr
    p "#{host}:#{port} connected"
    @selectables[client] = Rbev::IO.new()
    @selectables[client].io_register(IO.try_convert(client), :r, proc { read(client) })
    @selectables[client].io_start

    timer_cb = proc {
      p "timer ------------------------------"
      stop_client(client)
    }

    @selectables[client].timer_register(15.0, 0, timer_cb)
    @selectables[client].timer_start
  end

  def stop_client(client)
    @selectables[client].timer_stop
    @selectables[client].io_stop
    @selectables.delete client
  end

  def read(client)
    @selectables[client].timer_again
    if client.eof?
      client.close
      stop_client(client)
    else
      # FIXME
      data = client.read_nonblock(1460)
      p "recv data #{data}"
      client.write_nonblock(data)
    end
  end
end

host = '0.0.0.0'
port = 5359

DNSServer.new(host, port).run
