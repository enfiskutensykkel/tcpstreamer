#ifndef __BOOTSTRAP__
#define __BOOTSTRAP__

#include "streamer.h"
#include "utils.h"
#include <getopt.h>


typedef void (*callback_t)(struct timeval, pkt_t);


/* Register an argument to the streamer entry point. */
int register_argument(struct option argument);

/* TODO */
int register_callback(callback_t parser_func);

#endif
