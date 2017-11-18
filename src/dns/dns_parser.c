#include "dns_parser.h"

#include <arpa/nameser.h>
#include <errno.h>
#include <string.h>

const char *
hostname_from_question(ns_msg msg) {
  static char hostname[256] = {0};
  ns_rr rr;
  int rrnum, rrmax;
  const char *result;
  int result_len;
  rrmax = ns_msg_count(msg, ns_s_qd);
  if (rrmax == 0)
    return NULL;
  for (rrnum = 0; rrnum < rrmax; rrnum++) {
    if (ns_parserr(&msg, ns_s_qd, rrnum, &rr)) {
      printf("ns_parserr\n");
      return NULL;
    }
    result = ns_rr_name(rr);
    result_len = strlen(result) + 1;
    if (result_len > sizeof(hostname)) {
      printf("hostname too long: %s\n", result);
    }
    memset(hostname, 0, sizeof(hostname));
    memcpy(hostname, result, result_len);
    return hostname;
  }
  return NULL;
}


char *get_query_domain(const u_char *payload, size_t paylen, FILE *trace) {
  ns_msg msg;

  // Message too long
  if (ns_initparse(payload, paylen, &msg) < 0) {
    fputs(strerror(errno), trace);
    printf("\n");
    return NULL;
  }

  const char *host = hostname_from_question(msg);
  return host;
}
