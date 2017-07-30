# ip2socks

## Get start

```bash
git submodule init
git submodule update
git submodule foreach git pull

# --recursive

# archlinux
vagrant up --provider virtualbox
vagrant ssh
```

* `use-vc` in `/etc/resolv.conf`: Sets RES_USEVC in _res.options.  This option forces the use of TCP for DNS resolutions.

## Library

* C++ 11
* [lwip](https://github.com/FlowerWrong/lwip)
* [libev](http://software.schmorp.de/pkg/libev.html)
* [libyaml](https://github.com/yaml/libyaml)

## Remove submodule

```
Run git rm --cached <submodule name>
Delete the relevant lines from the .gitmodules file.
Delete the relevant section from .git/config.
Commit
Delete the now untracked submodule files.
Remove directory .git/modules/<submodule name>
```
