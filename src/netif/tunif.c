/**
 * based on lwip-contrib
 */
#include "netif/tunif.h"
#include "netif/socket_util.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "lwip/ip.h"


#if LWIP_IPV4 /* @todo: IPv6 */

#define IFNAME0 't'
#define IFNAME1 'n'

#ifndef TUNIF_DEBUG
#define TUNIF_DEBUG LWIP_DBG_OFF
#endif

#if defined(LWIP_UNIX_LINUX)
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>


#ifndef DEVTUN
#define DEVTUN "/dev/net/tun"
#endif

#define IP_ADDR_ARGS "addr add %d.%d.%d.%d/24 dev %s"
#define IP_UP_ARGS "link set %s up"
#define IP_BIN "/sbin/ip "
#elif defined(LWIP_UNIX_OPENBSD)
#define DEVTUN "/dev/tun0"

#define IP_ADDR_ARGS "addr add %d.%d.%d.%d/24 dev %s"
#define IP_UP_ARGS "link set %s up"
#define IP_BIN "/sbin/ip "
#endif

#if defined(LWIP_UNIX_MACH)

#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/sys_domain.h>
#include <netinet/ip.h>
#include <sys/kern_event.h>
#include <sys/ioctl.h>

#define IFCONFIG_ARGS "%s %d.%d.%d.%d %d.%d.%d.%d mtu %d netmask %d.%d.%d.%d up"

#endif /* LWIP_UNIX_MACH */

#define IFCONFIG_BIN "/sbin/ifconfig "

struct tunif {
    int fd;
};

#define BUFFER_SIZE 1500

/* Forward declarations. */
void tunif_input(struct netif *netif);

static err_t tunif_output(struct netif *netif, struct pbuf *p,
                          const ip4_addr_t *ipaddr);


#if defined(LWIP_UNIX_MACH)

static inline int utun_modified_len(int len) {
  if (len > 0)
    return (len > sizeof(u_int32_t)) ? len - sizeof(u_int32_t) : 0;
  else
    return len;
}

static int utun_write(int fd, void *buf, size_t len) {
  u_int32_t type;
  struct iovec iv[2];
  struct ip *iph;

  iph = (struct ip *) buf;

  if (iph->ip_v == 6)
    type = htonl(AF_INET6);
  else
    type = htonl(AF_INET);

  iv[0].iov_base = &type;
  iv[0].iov_len = sizeof(type);
  iv[1].iov_base = buf;
  iv[1].iov_len = len;

  return utun_modified_len(writev(fd, iv, 2));
}

static int utun_read(int fd, void *buf, size_t len) {
  u_int32_t type;
  struct iovec iv[2];

  iv[0].iov_base = &type;
  iv[0].iov_len = sizeof(type);
  iv[1].iov_base = buf;
  iv[1].iov_len = len;

  return utun_modified_len(readv(fd, iv, 2));
}

#endif /* LWIP_UNIX_MACH */

#if defined(LWIP_UNIX_MACH)
#define tun_read(...) utun_read(__VA_ARGS__)
#define tun_write(...) utun_write(__VA_ARGS__)
#endif /* LWIP_UNIX_MACH */

/*-----------------------------------------------------------------------------------*/
#if defined(LWIP_UNIX_MACH)

int utun_create(char *dev, u_int32_t unit) {
  int fd;
  struct ctl_info ctlInfo;
  struct sockaddr_ctl sc;

  memset(&ctlInfo, 0, sizeof(ctlInfo));
  if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >=
      sizeof(ctlInfo.ctl_name)) {
    printf("can not setup utun device: UTUN_CONTROL_NAME too long");
    return -1;
  }

  fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);

  if (fd == -1) {
    printf("socket[SYSPROTO_CONTROL]");
    return -1;
  }

  if (ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
    close(fd);
    printf("ioctl[CTLIOCGINFO]");
    return -1;
  }

  sc.sc_id = ctlInfo.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = unit + 1;

  if (connect(fd, (struct sockaddr *) &sc, sizeof(sc)) == -1) {
    close(fd);
    printf("connect[AF_SYS_CONTROL]");
    return -1;
  }

  // Get iface name of newly created utun dev.
  char utunname[20];
  socklen_t utunname_len = sizeof(utunname);
  if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, utunname, &utunname_len))
    return -1;
  memcpy(dev, utunname, strlen(utunname));
  return fd;
}

#endif /* LWIP_UNIX_MACH */

int tun_create(char *dev) {
  int fd = -1;
#if defined(LWIP_UNIX_LINUX)
  struct ifreq ifr;

  if ((fd = open(DEVTUN, O_RDWR)) < 0) {
    printf("open %s failed\n", DEVTUN);
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

  if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
    printf("failed to open tun device\n");
    close(fd);
    return -1;
  }
  strcpy(dev, ifr.ifr_name);

  printf("Open tun device: %s for reading...\n", ifr.ifr_name);
#endif

#if defined(LWIP_UNIX_MACH)
  for (u_int32_t unit = 0; unit < 256; ++unit) {
    fd = utun_create(dev, unit);
    if (fd >= 0)
      break;
  }
#endif /* LWIP_UNIX_MACH */

  return fd;
}

static void
low_level_init(struct netif *netif) {
  struct tunif *tunif;
  int ret = 0;
  char buf[1024];

#if defined(LWIP_UNIX_MACH)
  char tun_name[16];
#endif /* LWIP_UNIX_MACH */

#if defined(LWIP_UNIX_LINUX)
  char tun_name[IFNAMSIZ];
#endif
  tun_name[0] = '\0';


  tunif = (struct tunif *) netif->state;

  /* Obtain MAC address from network interface. */

  /* Do whatever else is needed to initialize interface. */

  tunif->fd = tun_create(tun_name);
  if (tunif->fd < 1) {
    perror("tunif_init failed\n");
    exit(1);
  }

  printf("tun name is %s\n", tun_name);

#if defined(LWIP_UNIX_MACH)
  // ifconfig $intf $local_tun_ip $remote_tun_ip mtu $mtu netmask 255.255.255.0 up
  snprintf(buf, 1024, IFCONFIG_BIN
    IFCONFIG_ARGS,
           tun_name,
           ip4_addr1(netif_ip4_gw(netif)),
           ip4_addr2(netif_ip4_gw(netif)),
           ip4_addr3(netif_ip4_gw(netif)),
           ip4_addr4(netif_ip4_gw(netif)),
           ip4_addr1(netif_ip4_gw(netif)),
           ip4_addr2(netif_ip4_gw(netif)),
           ip4_addr3(netif_ip4_gw(netif)),
           ip4_addr4(netif_ip4_gw(netif)),
           BUFFER_SIZE,
           ip4_addr1(netif_ip4_netmask(netif)),
           ip4_addr2(netif_ip4_netmask(netif)),
           ip4_addr3(netif_ip4_netmask(netif)),
           ip4_addr4(netif_ip4_netmask(netif))
  );
  ret = system(buf);
#endif /* LWIP_UNIX_MACH */
#if defined(LWIP_UNIX_LINUX)
  snprintf(buf, 1024, IP_BIN IP_ADDR_ARGS,
           ip4_addr1(netif_ip4_gw(netif)),
           ip4_addr2(netif_ip4_gw(netif)),
           ip4_addr3(netif_ip4_gw(netif)),
           ip4_addr4(netif_ip4_gw(netif)),
           tun_name);
  ret = system(buf);
  snprintf(buf, 1024, IP_BIN IP_UP_ARGS,
           tun_name);
  ret = system(buf);
#endif

  if (ret < 0) {
    perror("ifconfig failed");
    exit(1);
  }

  setnonblocking(tunif->fd);
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct tunif *tunif, struct pbuf *p) {
  char buf[BUFFER_SIZE];
  int ret;

  /* initiate transfer(); */

  pbuf_copy_partial(p, buf, p->tot_len, 0);

  /* signal that packet should be sent(); */
#if defined(LWIP_UNIX_MACH)
  ret = tun_write(tunif->fd, buf, p->tot_len);
#endif
#if defined(LWIP_UNIX_LINUX)
  ret = write(tunif->fd, buf, p->tot_len);
#endif
  if (ret == -1) {
    perror("tunif: write failed\n");
  }
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct tunif *tunif) {
  struct pbuf *p;
  ssize_t len;
  char buf[BUFFER_SIZE];

  /* Obtain the size of the packet and put it into the "len"
     variable. */
#if defined(LWIP_UNIX_MACH)
  len = tun_read(tunif->fd, buf, sizeof(buf));
#endif /* LWIP_UNIX_MACH */
#if defined(LWIP_UNIX_LINUX)
  len = read(tunif->fd, buf, sizeof(buf));
#endif
  if ((len <= 0) || (len > 0xffff)) {
    return NULL;
  }

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_LINK, (u16_t) len, PBUF_POOL);

  if (p != NULL) {
    pbuf_take(p, buf, (u16_t) len);
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
    printf("pbuf_alloc failed\n");
    return NULL;
  }

  return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * tunif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actuall transmission of the packet.
 *
 */
/*-----------------------------------------------------------------------------------*/
static err_t
tunif_output(struct netif *netif, struct pbuf *p,
             const ip4_addr_t *ipaddr) {
  struct tunif *tunif;
  LWIP_UNUSED_ARG(ipaddr);

  tunif = (struct tunif *) netif->state;

  return low_level_output(tunif, p);

}
/*-----------------------------------------------------------------------------------*/
/*
 * tunif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
tunif_input(struct netif *netif) {
  struct tunif *tunif;
  struct pbuf *p;

  tunif = (struct tunif *) netif->state;

  p = low_level_input(tunif);

  if (p == NULL) {
    LWIP_DEBUGF(TUNIF_DEBUG, ("tunif_input: low_level_input returned NULL\n"));
    return;
  }

  err_t err = netif->input(p, netif);
  if (err != ERR_OK) {
    printf("============================> tapif_input: netif input error %s\n", lwip_strerr(err));
    pbuf_free(p);
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * tunif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
tunif_init(struct netif *netif) {
  struct tunif *tunif;

  tunif = (struct tunif *) mem_malloc(sizeof(struct tunif));
  if (!tunif) {
    return ERR_MEM;
  }
  netif->state = tunif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = tunif_output;

  low_level_init(netif);
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_IPV4 */
