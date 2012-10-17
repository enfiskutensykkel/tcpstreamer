#ifndef __BOOTSTRAP__
#define __BOOTSTRAP__

#include "streamer.h"
#include <getopt.h>

/* Register an argument to the streamer entry point. */
int register_argument(struct option argument);

#endif
