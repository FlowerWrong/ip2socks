require 'ipaddr'
require 'socket'

module Ip
  class << self
    def ip2int(ip)
      IPAddr.new(ip).to_i
    end

    # family: AF_INET or AF_INET6
    def int2ip(int_ip, family = Socket::AF_INET)
      IPAddr.new(int_ip, family).to_s
    end
  end
end
