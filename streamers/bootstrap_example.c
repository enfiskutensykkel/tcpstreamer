#include "streamer.h"
#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>

/* Tell the project builder that bootstrap_example is our bootstrapper function */
#define BOOTSTRAP bootstrap_example 
#include "bootstrap.h"


/* A stub function serves as our entry point */
ssize_t example_entry_point(int socket_descriptor, ...)
{
	printf("Hello world from user entry point!\n"
			"Entry point argument: %x\n", socket_descriptor);

	return 0;
}


/* Our custom bootstrapper */
void bootstrap_example(streamer_t* entry_point, cancel_t* destructor)
{
	puts("Bootstrapping entry point...");

	/* Put address of entry point into pointer */
	*entry_point = &example_entry_point;
}
