#ifndef __BOOTSTRAP__
#define __BOOTSTRAP__

#include "streamer.h"
#include <getopt.h>

/* Register an argument to the streamer entry point. */
void register_argument(streamer_t streamer, struct option argument);

#endif
