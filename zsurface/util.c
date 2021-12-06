#include <stdio.h>

#include "internal.h"

WL_EXPORT void
zsurf_log(const char* fmt, ...)
{
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
}
