#include "streamer.h"
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>


/* Use this to emulate that our streamer is running */
static int run = 1;

/* A stub function serves as our entry point */
ssize_t example_entry_point(int socket_descriptor, ...)
{
	printf("Hello world from user entry point!\n"
			"Entry point argument: %x\n", socket_descriptor);

	/* Pretend that we are doing something */
	while (run);

	return 0;
}


STREAMER(example_entry_point, NULL)
