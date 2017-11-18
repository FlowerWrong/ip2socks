#include <unitypes.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

char *get_query_domain(const u_char *payload, int paylen, FILE *trace);

#ifdef __cplusplus
}
#endif
