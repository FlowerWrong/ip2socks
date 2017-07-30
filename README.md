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
cmake .
make

# OSX
sudo ./ip2socks --config=./scripts/config.example.yml --onshell=./scripts/darwin_setup_utun.sh --downshell=./scripts/darwin_down_utun.sh

# linux
sudo ./ip2socks --config=./scripts/config.example.yml --onshell=./scripts/linux_setup_tuntap.sh --downshell=./scripts/linux_down_tuntap.sh
```

#### There are 4 way to setup DNS query with tcp

* `use-vc` in `/etc/resolv.conf`: Sets RES_USEVC in _res.options.  This option forces the use of TCP for DNS resolutions.
* pdnsd
* lwip udp hooked, redirect to upstream tcp dns server via socks 5, config with `remote_dns_server`, you can just route your dns servers to tun or tap with `route` on OSX or `ip route` on Linux
* lwip udp hooked, redirect to upstream tcp dns server via socks 5, config with `remote_dns_server`, setup your dns to `addr`, eg `10.0.0.2`

## Library

* C++ 11
* [lwip](https://github.com/FlowerWrong/lwip)
* [libev](http://software.schmorp.de/pkg/libev.html)
* [libyaml](https://github.com/yaml/libyaml)

## Know bugs

* [x] too many `CLOSE_WAIT` to socks server, see `netstat -an | grep CLOSE_WAIT | wc -l`
* [ ] OSX receive data too often, eg: `brew update`, `brew upgrade`

## TODO

* [ ] fake DNS server
* [ ] http/https proxy server
* [x] lwip `keep-alive` support
* [x] lwip `SO_REUSEADDR` support
* [ ] TCP fast open with Linux kernel > 3.7.0
* [ ] socks 5 client UDP relay
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
