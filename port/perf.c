/**
 * based on lwip-contrib
 */
#include "arch/perf.h"

#include <stdio.h>

static FILE *f;

void
perf_print(unsigned long c1l, unsigned long c1h,
           unsigned long c2l, unsigned long c2h,
           char *key) {
  unsigned long sub_ms, sub_ls;

  sub_ms = c2h - c1h;
  sub_ls = c2l - c1l;
  if (c2l < c1l) sub_ms--;
  fprintf(f, "%s: %.8lu%.8lu\n", key, sub_ms, sub_ls);
  fflush(NULL);
}

void
perf_print_times(struct tms *start, struct tms *end, char *key) {
  fprintf(f, "%s: %lu\n", key, end->tms_stime - start->tms_stime);
  fflush(NULL);
}

void
perf_init(char *fname) {
  f = fopen(fname, "w");
}
	
