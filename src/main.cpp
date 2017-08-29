#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include "ev.h"
#include "yaml.h"

#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/timeouts.h"
#include "lwip/ip.h"
#include "lwip/ip4_frag.h"

#include "struct.h"
#include "util.h"

#if defined(LWIP_UNIX_LINUX)

#include "netif/tapif.h"

#endif

#include "netif/tunif.h"
#include "netif/etharp.h"

#include "udp_raw.h"
#include "tcp_raw.h"

// mruby
#include <mruby.h>
#include <mruby/compile.h>

// js
#include "duktape.h"
#include "duktape_socket/socket.h"

duk_context *ctx;

static duk_ret_t native_print(duk_context *ctx) {
  duk_push_string(ctx, " ");
  duk_insert(ctx, 0);
  duk_join(ctx, duk_get_top(ctx) - 1);
  printf("%s\n", duk_safe_to_string(ctx, -1));
  return 0;
}

static duk_ret_t native_adder(duk_context *ctx) {
  int i;
  int n = duk_get_top(ctx);  /* #args */
  double res = 0.0;

  for (i = 0; i < n; i++) {
    res += duk_to_number(ctx, i);
  }

  duk_push_number(ctx, res);
  return 1;  /* one return value */
}


// lua
#include "lua.hpp"
#include "lauxlib.h"
#include "lualib.h"

static lua_State *L = NULL;

int ladd(int x, int y) {
  int sum;

  lua_getglobal(L, "add");
  lua_pushinteger(L, x);
  lua_pushinteger(L, y);
  lua_call(L, 2, 1);

  sum = (int) lua_tointeger(L, -1);

  lua_pop(L, 1);

  return sum;
}


/* lwip host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;

static char *config_file;
static char *onshell_file;
static char *downshell_file;

void on_shell();

void down_shell();

/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

static struct option longopts[] = {
  /* turn on debugging output (if build with LWIP_DEBUG) */
  {"debug",     no_argument,       NULL, 'd'},
  /* help */
  {"help",      no_argument,       NULL, 'h'},
  /* config file */
  {"config",    required_argument, NULL, 'c'},
  /* onshell script */
  {"onshell",   required_argument, NULL, 'o'},
  /* downshell script */
  {"downshell", required_argument, NULL, 'n'},
  /* new command line options go here! */
  {NULL, 0,                        NULL, 0}
};
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)

static void
usage(void) {
  unsigned char i;
  printf("options:\n");
  for (i = 0; i < NUM_OPTS; i++) {
    printf("-%c --%s\n", longopts[i].val, longopts[i].name);
  }
}

#define BUFFER_SIZE 1514

void tuntap_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

void sigterm_cb(struct ev_loop *loop, ev_signal *watcher, int revents);

void sigint_cb(struct ev_loop *loop, ev_signal *watcher, int revents);

void sigusr2_cb(struct ev_loop *loop, ev_signal *watcher, int revents);

struct netif netif;

int
main(int argc, char **argv) {
  int ch;
  char ip_str[16] = {0}, nm_str[16] = {0}, gw_str[16] = {0};

  /* startup defaults (may be overridden by one or more opts) */
  IP4_ADDR(&gw, 10, 0, 0, 1);
  // ip4_addr_set_any(&gw);
  ip4_addr_set_any(&ipaddr);
  IP4_ADDR(&ipaddr, 10, 0, 0, 2);
  IP4_ADDR(&netmask, 255, 255, 255, 0);

  /* use debug flags defined by debug.h */
  debug_flags = LWIP_DBG_OFF;

  while ((ch = getopt_long(argc, argv, "dhc:o:n:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'd':
        debug_flags |= (LWIP_DBG_ON | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_FRESH | LWIP_DBG_HALT);
            break;
      case 'h':
        usage();
            exit(0);
      case 'c':
        config_file = optarg;
            break;
      case 'o':
        onshell_file = optarg;
            break;
      case 'n':
        downshell_file = optarg;
            break;
      default:
        usage();
            break;
    }
  }
  argc -= optind;
  argv += optind;

  if (config_file == NULL) {
    printf("Please provide config file\n");
    exit(0);
  }

  printf("config file %s, on shell file %s, down shell file %s\n", config_file, onshell_file, downshell_file);


  mrb_state *mrb = mrb_open();
  if (!mrb) { /* handle error */ }
  puts("Executing Ruby code from C!");
  mrb_load_string(mrb, "p 'hello world!'");
  FILE *mruby_file = fopen("./src/loop.rb", "r");
  if (mruby_file) {
    mrb_load_file(mrb, mruby_file);
    fclose(mruby_file);
  }
  mrb_close(mrb);


  /**
   * lua
   */
  L = luaL_newstate();
  luaL_openlibs(L);
  int ret = luaL_dofile(L, "./src/sum.lua");
  if (ret) {
    const char *pErrorMsg = lua_tostring(L, -1);
    std::cout << pErrorMsg << std::endl;
    lua_close(L);
    return -1;
  }
  lua_getglobal(L, "add");
  /* the first argument */
  lua_pushnumber(L, 41);
  /* the second argument */
  lua_pushnumber(L, 22);
  /* call the function with 2 arguments, return 1 result */
  lua_call(L, 2, 1);
  /* get the result */
  int sum = (int) lua_tonumber(L, -1);
  lua_pop(L, 1);
  /* print the result */
  printf("The result is %d\n", sum);
  lua_close(L);


  /**
   * js duktape engine
   */
  ctx = duk_create_heap_default();
  if (ctx) {
    duk_push_c_function(ctx, native_print, DUK_VARARGS);
    duk_put_global_string(ctx, "print");
    duk_push_c_function(ctx, native_adder, DUK_VARARGS);
    duk_put_global_string(ctx, "adder");

    socket_register(ctx);

    duk_eval_string(ctx, "print('2+3=' + adder(2, 3));");
    duk_pop(ctx);  /* pop eval result */
    duk_destroy_heap(ctx);
  }


  /**
   * yaml config parser start
   */
  FILE *fh = fopen(config_file, "r");
  yaml_parser_t parser;
  yaml_token_t token;   /* new variable */

  /* Initialize parser */
  if (!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);
  if (fh == NULL)
    fputs("Failed to open file!\n", stderr);

  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);

  /**
   * state = 0 = expect key
   * state = 1 = expect value
   */
  int state = 0;
  char **datap;
  char *tk;

  /* BEGIN new code */
  do {
    yaml_parser_scan(&parser, &token);
    switch (token.type) {
      /* Stream start/end */
      case YAML_STREAM_START_TOKEN:
        puts("STREAM START");
            break;
      case YAML_STREAM_END_TOKEN:
        puts("STREAM END");
            break;
            /* Token types (read before actual token) */
      case YAML_KEY_TOKEN:
        printf("(Key token)   ");
            state = 0;
            break;
      case YAML_VALUE_TOKEN:
        printf("(Value token) ");
            state = 1;
            break;
            /* Block delimeters */
      case YAML_BLOCK_SEQUENCE_START_TOKEN:
        puts("<b>Start Block (Sequence)</b>");
            break;
      case YAML_BLOCK_ENTRY_TOKEN:
        puts("<b>Start Block (Entry)</b>");
            break;
      case YAML_BLOCK_END_TOKEN:
        puts("<b>End block</b>");
            break;
            /* Data */
      case YAML_BLOCK_MAPPING_START_TOKEN:
        puts("[Block mapping]");
            break;
      case YAML_SCALAR_TOKEN:
        printf("scalar %s \n", token.data.scalar.value);
            tk = (char *) token.data.scalar.value;
            if (state == 0) {
              if (strcmp(tk, "ip_mode") == 0) {
                datap = &conf->ip_mode;
              } else if (strcmp(tk, "dns_mode") == 0) {
                datap = &conf->dns_mode;
              } else if (strcmp(tk, "socks_server") == 0) {
                datap = &conf->socks_server;
              } else if (strcmp(tk, "socks_port") == 0) {
                datap = &conf->socks_port;
              } else if (strcmp(tk, "remote_dns_server") == 0) {
                datap = &conf->remote_dns_server;
              } else if (strcmp(tk, "remote_dns_port") == 0) {
                datap = &conf->remote_dns_port;
              } else if (strcmp(tk, "local_dns_port") == 0) {
                datap = &conf->local_dns_port;
              } else if (strcmp(tk, "relay_none_dns_packet_with_udp") == 0) {
                datap = &conf->relay_none_dns_packet_with_udp;
              } else if (strcmp(tk, "custom_domian_server_file") == 0) {
                datap = &conf->custom_domian_server_file;
              } else if (strcmp(tk, "gw") == 0) {
                datap = &conf->gw;
              } else if (strcmp(tk, "addr") == 0) {
                datap = &conf->addr;
              } else if (strcmp(tk, "netmask") == 0) {
                datap = &conf->netmask;
              } else {
                printf("Unrecognised key: %s\n", tk);
              }
            } else {
              *datap = strdup(tk);
            }
            break;
            /* Others */
      default:
        printf("Got token of type %d\n", token.type);
    }
    if (token.type != YAML_STREAM_END_TOKEN)
      yaml_token_delete(&token);
  } while (token.type != YAML_STREAM_END_TOKEN);
  yaml_token_delete(&token);
  /* END new code */

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);
  /**
   * yaml config parser end
   */

  /**
   * if config, overside default value
   */
  if (conf->gw != NULL) {
    ip4addr_aton(conf->gw, &gw);
  }
  if (conf->addr != NULL) {
    ip4addr_aton(conf->addr, &ipaddr);
  }
  if (conf->netmask != NULL) {
    ip4addr_aton(conf->netmask, &netmask);
  }

  if (conf->ip_mode == NULL) {
    memcpy(conf->ip_mode, "tun", 3);
  } else {
#if defined(LWIP_UNIX_MACH)
    if (strcmp("tap", conf->ip_mode) == 0) {
      printf("apple does not support tap mode!!!\n");
      exit(0);
    }
#endif /* LWIP_UNIX_MACH */
  }
  if (conf->dns_mode == NULL) {
    memcpy(conf->dns_mode, "tcp", 3);
  }

  strncpy(ip_str, ip4addr_ntoa(&ipaddr), sizeof(ip_str));
  strncpy(nm_str, ip4addr_ntoa(&netmask), sizeof(nm_str));
  strncpy(gw_str, ip4addr_ntoa(&gw), sizeof(gw_str));
  printf("Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);


  if (conf->custom_domian_server_file != NULL) {
    std::string line;
    std::string file_sp(";");
    std::string sp("/");

    std::vector<std::vector<std::string>> domains;

    std::vector<std::string> files;
    std::string ffs(conf->custom_domian_server_file);
    split(ffs, file_sp, &files);
    for (int i = 0; i < files.size(); ++i) {
      if (!files.at(i).empty()) {
        std::ifstream chndomains(files.at(i));
        if (chndomains.is_open()) {
          while (getline(chndomains, line)) {
            if (!line.empty() && line.substr(0, 1) != "#") {
              std::vector<std::string> v;
              split(line, sp, &v);
              domains.push_back(v);
            }
          }
          chndomains.close();
        } else {
          std::cout << "Unable to open dns domain file " << files.at(i) << std::endl;
        }
      }
    }
    conf->domains = domains;
  }


  /* lwip/src/core/init.c */
  lwip_init();

  if (strcmp(conf->ip_mode, "tun") == 0) {
    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tunif_init, ip_input); // IPV4 IPV6 TODO
  } else {
#if defined(LWIP_UNIX_LINUX)
    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);
#endif
  }
  netif_set_default(&netif);
  netif_set_link_up(&netif);
  /* lwip/core/netif.c */
  netif_set_up(&netif);
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&netif, 1);
#endif

  udp_raw_init();
  tcp_raw_init();

  struct ev_io *tuntap_io = (struct ev_io *) mem_malloc(sizeof(struct ev_io));
  if (tuntap_io == NULL) {
    printf("tuntap_io: out of memory for tuntap_io\n");
    return -1;
  }
  struct ev_loop *loop = ev_default_loop(0);

  struct tuntapif *tuntapif;
  tuntapif = (struct tuntapif *) ((&netif)->state);

  /**
   * signal start
   */
  // eg: kill
  ev_signal signal_term_watcher;
  ev_signal_init(&signal_term_watcher, sigterm_cb, SIGTERM);
  ev_signal_start(loop, &signal_term_watcher);

  // eg: ctrl + c
  ev_signal signal_int_watcher;
  ev_signal_init(&signal_int_watcher, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_int_watcher);

  ev_signal signal_usr2_watcher;
  ev_signal_init(&signal_usr2_watcher, sigusr2_cb, SIGUSR2);
  ev_signal_start(loop, &signal_usr2_watcher);
  /**
   * signal end
   */

  ev_io_init(tuntap_io, tuntap_read_cb, tuntapif->fd, EV_READ);
  ev_io_start(loop, tuntap_io);


  // TODO
  sys_check_timeouts();

  /**
   * setup shell scripts
   */
  on_shell();

  std::cout << "Ip2socks started!" << std::endl;

  return ev_run(loop, 0);
}

void down_shell() {
  /**
   * down shell scripts
   */
  if (downshell_file != NULL) {
    std::string sh("sh ");
    sh.append(downshell_file);
    sh.append(" ");

    if (strcmp(conf->ip_mode, "tun") == 0) {
      sh.append(conf->gw);
    } else {
      sh.append(conf->addr);
    }

    printf("exec down shell %s\n", sh.c_str());

    system(sh.c_str());
  }
}

void on_shell() {
  /**
   * setup shell scripts
   */
  if (onshell_file != NULL) {
    std::string sh("sh ");
    sh.append(onshell_file);
    sh.append(" ");

    if (strcmp(conf->ip_mode, "tun") == 0) {
      sh.append(conf->gw);
    } else {
      sh.append(conf->addr);
    }

    printf("exec setup shell %s\n", sh.c_str());

    system(sh.c_str());
  }
}

void sigterm_cb(struct ev_loop *loop, ev_signal *watcher, int revents) {
  printf("SIGTERM handler called in process!!!\n");
  down_shell();
  ev_break(loop, EVBREAK_ALL);
}

void sigint_cb(struct ev_loop *loop, ev_signal *watcher, int revents) {
  printf("SIGINT handler called in process!!!\n");
  down_shell();
  ev_break(loop, EVBREAK_ALL);
}

void sigusr2_cb(struct ev_loop *loop, ev_signal *watcher, int revents) {
  printf("SIGUSR2 handler called in process!!! TODO reload config.\n");
}

void tuntap_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  if (strcmp(conf->ip_mode, "tun") == 0) {
    tunif_input(&netif);
  } else {
#if defined(LWIP_UNIX_LINUX)
    tapif_input(&netif);
#endif
  }
}
