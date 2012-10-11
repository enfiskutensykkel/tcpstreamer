#ifndef __ENTRY__
#define __ENTRY__

#include <stdarg.h>
#include <stdint.h>

typedef int64_t (*streamer_t)(int socket_desc, uint16_t duration, ...);

extern streamer_t entry_points[];

#endif
