//
// Created by king Yang on 2017/8/22.
//

/* Get Javascript compatible 'now' timestamp (millisecs since 1970). */
static double get_now(void) {
  struct timeval tv;
  int rc;

  rc = gettimeofday(&tv, NULL);
  if (rc != 0) {
    /* Should never happen, so return whatever. */
    return 0.0;
  }
  return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
}
