#include "streamer.h"
#include "bootstrap.h"
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <getopt.h>


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



void example_sighandler(streamer_t instance)
{
	if (instance == &example_entry_point)
		run = 0;
}



void example_bootstrap(streamer_t entry_point)
{
	int i;
	struct option options[] = {
		{"bufsize", 0, 0, 0},
		{"interval", 1, 0, 0},
		{0, 0, 0, 0}
	};

	if (entry_point == &example_entry_point) {
		for (i = 0; options[i].name != NULL; ++i)
			register_option(&example_entry_point, options[i]);
	}
}


STREAMER(example_entry_point, example_sighandler, example_bootstrap)
