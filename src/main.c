#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include "autogen.h"

/* Create a string out of a numerical define */
#define STRINGIFY(str) #str
#define NUM_2_STR(str) STRINGIFY(str)



int main(int argc, char **argv)
{
	int i, num_streamers;

	/* Count the number of possible streamers */
	for (num_streamers = 0; streamers[num_streamers].bootstrapper != NULL; ++num_streamers);
	
	// This is a stub at the moment
	goto give_usage;

	exit(0);

give_usage:
	fprintf(stderr,
			"Usage: %s [-p <port>] -s <streamer> [<args>] <host>\n"
			"       %s [-p <port>]\n"
			"  -s\tStart streamer instance <streamer> to <host>.\n"
			"  -p\tUse <port> instead of default port.\n"
			,
			argv[0], argv[0]);

	fprintf(stderr, "\nAvailable streamer instances are:\n");
	for (i = 0; streamers[i].bootstrapper != NULL; ++i) {
		fprintf(stderr, "\t%s\n", streamers[i].instance_name);
	}

	exit(1);
}
