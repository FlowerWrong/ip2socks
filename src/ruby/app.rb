require_relative 'http_proxy_server'
require_relative 'dns_server'

DNSServer.new('0.0.0.0', 5359).run
HTTPProxyServer.new('0.0.0.0', 5201).run
