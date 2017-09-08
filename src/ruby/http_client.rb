require_relative 'init'

require 'socket'
require 'awesome_print'
require 'http/parser'

class HTTPClient
  attr_accessor :headers, :body, :socket, :ev_io, :server

  def initialize(socket, server)
    @server = server
    @socket = socket
    @parser = Http::Parser.new(self)

    @ev_io = Rbev::IO.new()
    @ev_io.io_register(IO.try_convert(client), :r, proc { read })
    @ev_io.io_start

    timer_cb = proc {
      ap "timer ------------------------------"
      stop_client(client)
    }

    @ev_io.timer_register(15.0, 0, timer_cb)
    @ev_io.timer_start
  end

  def stop_client
    @socket.close
    @socket.timer_stop
    @socket.io_stop
    @server.selectables.delete @socket
  end

  def read
    @socket.timer_again
    if @socket.eof?
      stop_client
    else
      if @socket.respond_to?('read_nonblock')
        data = @socket.read_nonblock(1460)
        ap "recv data #{data}"
        @parser << data
      end
    end
  end

  def on_message_begin
    @headers = nil
    @body = ''
  end

  def on_headers_complete(headers)
    @headers = headers
  end

  def on_body(chunk)
    @body << chunk
  end

  def on_message_complete
    p [@headers, @body]

    @socket.write_nonblock("HTTP/1.1 200 OK\r\nCache-Control: no-cache, private\r\nContent-Length: 107\r\nDate: Mon, 24 Nov 2014 10:21:21 GMT\r\n\r\n")
  end
end
