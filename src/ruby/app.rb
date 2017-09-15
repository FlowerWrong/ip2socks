require_relative 'http_proxy'
require_relative 'dns'
require 'socket'

DNSServer.new('0.0.0.0', 5359).run
