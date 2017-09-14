require_relative './ip.rb'
require_relative './adler32.rb'
require_relative './init.rb'

require 'ipaddress'


class IpPool
  DnsIPPoolMaxSpace = 0x3ffff
  attr_accessor :min, :max, :space, :flags

  # ip:     10.192.0.1
  # subnet: 10.192.0.0/16
  def initialize(ip, subnet)
    subnet_ip = IPAddress subnet
    @min = Ip.ip2int(subnet_ip.address) + 1
  	@max = @min + (~Ip.ip2int(subnet_ip.netmask) & 0xFFFFFFFF)

  	@space = max - min

    if @space > DnsIPPoolMaxSpace
      @space = DnsIPPoolMaxSpace
    end

    @flags = {}
    flag_ip(ip)
  end

  def flag_ip(ip)
    index = Ip.ip2int(ip) - @min
    if index < @space
      @flags[index] = true
    end
  end

  def domain2ip(domain)
    index = Adler32.checksum(domain) % @space
    if @flags[index]
      p "[dns] #{domain} is not in main index: #{index}"
      return nil
    else
      @flags[index] = true
      return Ip.int2ip(@min + index)
    end
  end

  def release_ip(ip)
    index = Ip.ip2int(ip) - @min
    if index < @space
      @flags[index] = false
    end
  end

  def contains?(ip)
    index = Ip.ip2int(ip) - @min
    index < @space
  end
end
