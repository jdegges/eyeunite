#ifndef EU_DEBUG_H
#define EU_DEBUG_H

#include <stdio.h>
#include <time.h>

FILE *DEBUGFP;

#define print_error(format, ...){ \
  if (NULL == DEBUGFP) \
    DEBUGFP = stderr; \
  fprintf (DEBUGFP, "[%lu %s:%d in %s] ", time (NULL), __FILE__, __LINE__, \
                                          __func__); \
  fprintf (DEBUGFP, format, ##__VA_ARGS__); \
  fprintf (DEBUGFP, "\n");\
  fflush (DEBUGFP); \
}

#endif
