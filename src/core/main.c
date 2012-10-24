#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pcap.h>
#include "instance.h"
#include "utils.h"
#include "bootstrap.h"

/* Defines for pretty-print */
#ifdef ANSI
#define B "\033[1m"
#define U "\033[4m"
#define R "\033[0m"
#else
#define B ""
#define U ""
#define R ""
#endif




/* Print program usage */
static void give_usage(FILE *out, char *prog_name, char *streamer);




int main(int argc, char **argv)
{
	char *streamer_name = NULL;         // name of streamer
	streamer_t streamer_entry = NULL;   // pointer streamer entry point
	callback_t streamer_init = NULL;    // pointer to streamer initialization function


	/* Parse command line options and arguments */
	int opt, help = 0, optidx; struct option *optvec = NULL;
	while ((opt = getopt_long(argc, argv, ":ht:p:s:", optvec, &optidx)) != -1) {
		switch (opt) {
			case ':': // missing value
				if (optopt == '\0')
					fprintf(stderr, "Argument %s requires a value\n", argv[optind-1]);
				else
					fprintf(stderr, "Option -%c requires a parameter\n", optopt);
				give_usage(stderr, argv[0], streamer_name);
				goto cleanup_and_die;

			case '?': // unknown flag
				if (optopt == '\0')
					fprintf(stderr, "Unknown argument: %s\n", argv[optind-1]);
				else
					fprintf(stderr, "Unknown options: -%c\n", optopt);
				give_usage(stderr, argv[0], streamer_name);
				goto cleanup_and_die;

			case 'h': // show help
				help = 1;
				break;

			case 's': // select streamer
				if (streamer_entry != NULL) {
					fprintf(stderr, "Streamer is already selected\n");
					goto cleanup_and_die;
				}

				/* Load streamer */
				if (load_streamer(optarg, &streamer_entry, &streamer_init) < 0) {
					fprintf(stderr, "No such streamer: %s\n", optarg);
					goto cleanup_and_die;
				}
				streamer_name = optarg;

				/* Initialize streamer */
				if (streamer_init != NULL)
					streamer_init(streamer_entry);
				break;
		}
	}

	if (streamer_entry != NULL) {
		streamer_entry(0xdeadbeef, 0, 0);
	}

	/* Uninitialize streamer */
	if (streamer_destroy != NULL)
		streamer_destroy(streamer_entry);

	exit(0);

cleanup_and_die:
	exit(1);
}



/* Print program usage */
static void give_usage(FILE *fp, char *name, char *streamer)
{
	if (streamer == NULL) {
		fprintf(fp,
				"Usage: %s [" U "options" R "] [" U "arguments" R "...] " U "host" R "\n"
				"   or: %s [" U "options" R "]\n"
				"\nAvailable options:\n"
				"  -h  "   "        "   "\tShow usage, use in combination with -s for more.\n"
				"  -p  " U "port"     R "\tUse specified " U "port" R " instead of default port.\n"
				"  -v  " U "level"    R "\tSet verbosity to " U "level" R " (0=quiet, 1=normal, 2=verbose).\n"
				"Streaming options:\n"
				"  -s  " U "streamer" R "\tSelect " U "streamer" R " type.\n"
				"  -t  " U "duration" R "\tRun streamer for " U "duration" R " (seconds).\n"
				,
				name, name);
	} else {
		fprintf(fp, "Usage: %s " B "-s %s" R " " U "arguments" R "... " U "host" R "\n",
				name, streamer);
	}
}
