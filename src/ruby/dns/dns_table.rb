class DnsTable
  attr_accessor :ip_pool, :records, :fake_ips

  def initialize(ip_pool)
    @ip_pool = ip_pool
    @fake_ips = {}
  end

  def add(domain)
    fake_ip = @ip_pool.domain2ip(domain)
    if !fake_ip.nil?
      @fake_ips[domain] = fake_ip
    end
  end

  def get_fake_ip(domain)
    @fake_ips[domain]
  end

  def get_domain(fake_ip)
    @fake_ips.each do |key, value|
      return key if value == fake_ip
    end
    return nil
  end
end
