require_relative 'http_proxy'
require 'socket'

HTTPProxyServer.new('0.0.0.0', 5359).run
