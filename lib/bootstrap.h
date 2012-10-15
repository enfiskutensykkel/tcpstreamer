#ifndef __BOOTSTRAP__
#define __BOOTSTRAP__

#include "streamer.h"
#include <getopt.h>

/* Register an option to a streamer implementation. */
void register_option(streamer_t streamer, struct option option);

#endif
