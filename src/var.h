#define SLASH "/"
#define BUFFER_SIZE 1514
#define UDP_BUFFER_SIZE 1460
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)
#define container_of(ptr, type, member) ({      \
  const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
  (type *)( (char *)__mptr - offsetof(type,member) );})


// ==========socks 5==========
// socks5 version
#define SOCKS5_VERSION 0x05
// socks5 command
#define SOCKS5_CMD_CONNECT 0x01
#define SOCKS5_CMD_BIND 0x02
#define SOCKS5_CMD_UDPASSOCIATE 0x03

// socks5 address type
#define SOSKC5_ADDRTYPE_IPV4 0x01
#define SOSKC5_ADDRTYPE_DOMAIN 0x03
#define SOSKC5_ADDRTYPE_IPV6 0x04
