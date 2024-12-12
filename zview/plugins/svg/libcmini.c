#include "strchr.c"
#include "strstr.c"

#include <limits.h>

#include "strtol.c"

#define UNSIGNED 1
#include "strtol.c"

#ifndef __PUREC__
#define UNSIGNED 1
#define QUAD
#include "strtol.c"
#endif
