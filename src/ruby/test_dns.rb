require_relative 'init'
require 'socket'
require 'awesome_print'

client = UDPSocket.new
client.send 'hi', 0, '127.0.0.1', 5359

ap client.recvfrom(1460)
