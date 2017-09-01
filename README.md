# ip2socks

## Support now

#### OSX

#### Linux

* ubuntu tested
* archlinux tested

## Get start

```bash
git clone git@github.com:FlowerWrong/ip2socks.git --recursive

git submodule init
git submodule update

# ubuntu vm
vagrant up --provider virtualbox
vagrant ssh
```

#### Compile with C++ 11 and cmake

```bash
# build duktape
pip2 install PyYAML
cd duktape
python2 util/dist.py

cmake .
make

# OSX
sudo ./ip2socks --config=./scripts/config.example.yml --onshell=./scripts/darwin_setup_utun.sh --downshell=./scripts/darwin_down_utun.sh

# linux
sudo ./ip2socks --config=./scripts/config.example.yml --onshell=./scripts/linux_setup_tuntap.sh --downshell=./scripts/linux_down_tuntap.sh
```

#### ip mode

* tun or utun
* tap

#### dns mode

* tcp: just dns with port you set `local_dns_port` redirect to tcp, other flow will be try to send to remote via socks 5 udp tunnel
* udp: just dns with port you set `local_dns_port` redirect to udp, other flow will be send to remote via socks 5 udp tunnel

#### There are 5 way to setup DNS query with tcp

* `use-vc` in `/etc/resolv.conf`: Sets RES_USEVC in _res.options.  This option forces the use of TCP for DNS resolutions.
* pdnsd
* lwip udp hooked, redirect to upstream tcp dns server via socks 5, config with `remote_dns_server`, you can just route your dns servers to tun or tap with `route` on OSX or `ip route` on Linux
* lwip udp hooked, redirect to upstream tcp dns server via socks 5, config with `remote_dns_server`, setup your dns to `addr`, eg `10.0.0.2`
* lwip udp hooked, set you dns to remote, eg: `8.8.8.8`

## Library

* C++ 11
* [lwip](https://github.com/FlowerWrong/lwip)
* [libev](http://software.schmorp.de/pkg/libev.html)
* [libyaml](https://github.com/yaml/libyaml)

## Know bugs

* [x] too many `CLOSE_WAIT` to socks server, see `netstat -an | grep CLOSE_WAIT | wc -l`
* [x] OSX receive data too often, eg: `brew update`, `brew upgrade`
* [x] if `ERR_QUIC_PROTOCOL_ERROR`, go to `chrome://flags/` disable quic
* [x] ns_initparse `Message too long` bug
* [ ] (libev) select: Invalid argument with setnoneblocking [socket-options-so-reuseaddr-and-so-reuseport](https://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t)
* [ ] tcp_raw_error is -14(ERR_RST): Connection reset.

## TODO

* [ ] DNS cache
* [x] `block` rule support, just close it
* [ ] dnsmasq `address=/test.com/127.0.0.1` support
* [x] `domain`, `domain_keyword`, `domain_suffix` (ip_cidr, geoip) rule support
* [x] timeout
* [ ] log
* [ ] fake DNS server
* [ ] http/https proxy server
* [ ] OSX route batch insert
* [x] lwip `keep-alive` support
* [x] lwip `SO_REUSEADDR` support
* [ ] TCP fast open with Linux kernel > 3.7.0
* [x] socks 5 client UDP relay
* [ ] FreeBSD support
* [ ] Android support
* [ ] iOS support
* [ ] ipv6 support

## How to remove submodule?

```
Run git rm --cached <submodule name>
Delete the relevant lines from the .gitmodules file.
Delete the relevant section from .git/config.
Commit
Delete the now untracked submodule files.
Remove directory .git/modules/<submodule name>
```
