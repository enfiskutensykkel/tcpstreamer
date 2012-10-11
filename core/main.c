#include "entry.h"
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



/* Create a string out of a numerical define */
#define STRINGIFY(str) #str
#define NUM_2_STR(str) STRINGIFY(str)



int main(int argc, char **argv)
{
	int opt;
	unsigned int byte_size = DEF_SIZE, // default segment size
				 duration = DEF_DUR,   // default stream duration
				 interval = 0;         // default intertransmission interval
	char *port = NUM_2_STR(DEF_PORT),  // default connection port
		 *sptr = NULL;
	

	/* Parse program options */
	while ((opt = getopt(argc, argv, ":d:i:n:p:")) != -1) {
		sptr = NULL;

		switch (opt) {
			case 'd':
				duration = strtoul(optarg, &sptr, 10);
				break;

			case 'i':
				interval = strtoul(optarg, &sptr, 10);
				break;

			case 'n':
				byte_size = strtoul(optarg, &sptr, 10);
				break;

			case 'p':
				if (strtoul(optarg, &sptr, 0) > 0xffff)
					goto give_usage;
				port = optarg;
				break;
		}

		if (sptr == NULL || *sptr != '\0')
			goto give_usage;
	}
	if (argc - optind > 1)
		goto give_usage;

	
	entry_points[0](0, 0);

	exit(0);

give_usage:
	fprintf(stderr,
			"Usage: %s [-d <duration>] [-i <interval] [-n <size>] [-p <port>] <host>\n"
			"       %s [-p <port>]\n"
			"  -d\tStream duration (seconds).\n"
			"  -i\tInter-transmission interval (milliseconds).\n"
			"  -n\tTransmission chunk size (bytes).\n"
			"  -p\tUse <port> instead of default port.\n"
			"\nIf <host> isn't given, the program will start as a receiver instance.\n"
			"If neither -i nor -n is set, the program will start a greedy sender.\n"
			,
			argv[0], argv[0]);
	exit(1);
}
