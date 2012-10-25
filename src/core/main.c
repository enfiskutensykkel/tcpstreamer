#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pcap.h>
#include <assert.h>
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



/* Streamer parameter list */
static struct option *streamer_params = NULL;

/* Streamer arguments */
static char const **streamer_args = NULL;

/* Pointer to streamer entry point */
static streamer_t streamer_entry = NULL;

/* Pointer to streamer bootstrapper */
static callback_t streamer_init = NULL;

/* Are we running */
static int streamer_state = 0;





/* Signal handler to catch interrupts */
static void handle_signal()
{
	streamer_state = 0;
}



/* Register streamer argument */
int register_argument(char const *name, int *ref, int val)
{
	struct option empty = { 0, 0, 0, 0 };
	int i;

	/* Verify that the context is correct */
	assert(streamer_entry != NULL);

	for (i = 0; streamer_params != NULL && streamer_params[i].name != NULL; ++i)
		if (strcmp(streamer_params[i].name, name) == 0)
			return -1;

	if ((streamer_params = realloc(streamer_params, sizeof(struct option) * (i+2))) == NULL)
		return -2;

	streamer_params[i].name = name;
	streamer_params[i].has_arg = ref == NULL;
	streamer_params[i].flag = ref;
	streamer_params[i].val = val;

	memcpy(&streamer_params[i+1], &empty, sizeof(struct option));

	return i;
}



/* Initialize streamer arguments */
static int bootstrap_streamer(void **handle, char const *streamer)
{
	int i, n;

	/* Load streamer */
	if (load_streamer(handle, streamer, &streamer_entry, &streamer_init) < 0)
		return -1;

	/* Bootstrap streamer */
	if (streamer_init != NULL)
		streamer_init(streamer_entry);

	if (streamer_params != NULL) {
		/* Count number of arguments that require value */
		for (n = 0; streamer_params[n].name != NULL; ++n);

		/* Initialize argv-style arguments for streamer */
		if ((streamer_args = malloc(sizeof(char*) * n)) == NULL)
			return -2;
		for (i = 0; i < n; ++i)
			streamer_args[i] = NULL;

	} else {
		streamer_params = NULL;
		streamer_args = NULL;
	}

	return 0;
}



/* Print program usage */
static void give_usage(char *prog_name, char *streamer);




int main(int argc, char **argv)
{
	int i, sock_fd = -1;
	void *handle = NULL;
	char *streamer_name = NULL;
	unsigned duration = DEF_DUR;
	char *port = DEF_2_STR(DEF_PORT), *host = NULL, *sptr = NULL;
	char hostname[INET_ADDRSTRLEN];
	struct sockaddr_in addr;


	/* Parse command line options and arguments */
	int opt, help = 0, optidx = -1; 
	while ((opt = getopt_long(argc, argv, ":ht:p:s:", streamer_params, &optidx)) != -1) {
		switch (opt) {
			case ':': // missing value
				if (argv[optind-1][1] == '-') {
					fprintf(stderr, "Argument %s requires a value\n", argv[optind-1]);
					goto cleanup_and_die;
				} else
					fprintf(stderr, "Option -%c requires a parameter\n", optopt);
				break;

			case '?': // unknown flag
				if (optopt == '\0' || optopt == '-')
					fprintf(stderr, "Unknown argument: %s\n", argv[optind-1]);
				else
					fprintf(stderr, "Unknown option: -%c\n", optopt);
				break;

			case 'h': // show help
				help = 1;
				break;

			case 'p': // port
				sptr = NULL;
				if (strtoul(optarg, &sptr, 0) > 0xffff || sptr == NULL || *sptr != '\0') {
					fprintf(stderr, "Option -p requires a valid port number\n");
					goto cleanup_and_die;
				}
				port = optarg;
				break;

			case 't': // duration
				sptr = NULL;
				duration = strtoul(optarg, &sptr, 10);
				if (sptr == NULL || *sptr != '\0') {
					fprintf(stderr, "Option -t requires a valid number of seconds\n");
					goto cleanup_and_die;
				}
				break;

			case 's': // select streamer
				if (streamer_entry != NULL) {
					fprintf(stderr, "Streamer is already selected\n");
					goto cleanup_and_die;
				}

				if (bootstrap_streamer(&handle, optarg) < 0) {
					fprintf(stderr, "No such streamer: %s\n", optarg);
					goto cleanup_and_die;
				}
				streamer_name = optarg;
				break;

			default:
				if (streamer_args[optidx] != NULL) {
					fprintf(stderr, "Argument %s is already set\n", argv[optind-1]);
					goto cleanup_and_die;
				}
				streamer_args[optidx] = optarg;
				break;

		}
		optidx = -1;
	}
	if (help) {
		give_usage(argv[0], streamer_name);
		goto cleanup_and_die;
	}
	for (i = 0; streamer_params != NULL && streamer_params[i].name != NULL; ++i) {
		if (streamer_params[i].flag == NULL && streamer_params[i].val != 0 && streamer_args[i] == NULL) {
			fprintf(stderr, "Missing argument: --%s\n", streamer_params[i].name);
			goto cleanup_and_die;
		}
	}


	/* Create signal handlers */
	assert(signal(SIGINT, (void (*)(int)) &handle_signal) != SIG_ERR);
	assert(signal(SIGPIPE, (void (*)(int)) &handle_signal) != SIG_ERR);



	/* Create socket descriptor and start instance */
	streamer_state = 1;
	if (streamer_entry == NULL) {
		
		fprintf(stdout, "Starting receiver.\n");
		if ((sock_fd = create_socket(NULL, port)) < 0) {
			fprintf(stderr, "Unable to bind to port %s\n", port);
			goto cleanup_and_die;
		}
		fprintf(stdout, "Accepting connections on port %s\n", port);

		/* Start receiver instance */
		receiver(sock_fd, &streamer_state);

	} else if (argc - optind > 0) {

		host = argv[optind];
		fprintf(stdout, "Streamer %s selected.\n", streamer_name);
		if ((sock_fd = create_socket(host, port)) < 0) {
			fprintf(stderr, "Unable to connect to %s\n", host);
			goto cleanup_and_die;
		}

		lookup_addr(sock_fd, NULL, &addr);
		lookup_name(addr, hostname, sizeof(hostname));
		fprintf(stdout, "Successfully connected to %s\n", hostname);

		/* Start streamer instance */
		i = streamer(streamer_entry, duration, sock_fd, &streamer_state, streamer_args);
		fprintf(stdout, "Streamer exited with status code: %d\n", i);

	} else {
		fprintf(stderr, "Streamer selected, but no host given\n");
		goto cleanup_and_die;
	}

	/* Clean up and exit gracefully */
	if (sock_fd >= 0)
		close(sock_fd);
	free(streamer_params);
	free(streamer_args);
	unload_streamer(handle);

	exit(0);


cleanup_and_die:
	if (errno != 0) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	if (sock_fd >= 0)
		close(sock_fd);
	free(streamer_params);
	free(streamer_args);
	unload_streamer(handle);
	exit(1);
}





/* Print program usage */
static void give_usage(char *name, char *streamer)
{
	int i;
	FILE *stream = stdout;

	if (streamer == NULL) {
		/* Program options */
		fprintf(stream,
				"Usage: %s [" U "options" R "] [" U "arguments" R "...] " U "host" R "\n"
				"   or: %s [" U "options" R "]\n"
				"\nAvailable options:\n"
				"  -h  "   "        "   "\tShow usage, use in combination with -s for more.\n"
				"  -p  " U "port"     R "\tUse specified " U "port" R " instead of default port.\n"
				"  -v  " U "level"    R "\tSet verbosity to " U "level" R " (0=quiet, 1=normal, 2=verbose).\n"
				"Streaming options:\n"
				"  -s  " U "streamer" R "\tSelect " U "streamer" R ".\n"
				"  -t  " U "duration" R "\tRun streamer for " U "duration" R " (seconds).\n"
				,
				name, name);
	} else {
		/* Streamer specific options */
		fprintf(stream, "Usage: %s " B "-s %s" R " [" U "arguments" R "...] " U "host" R "\n",
				name, streamer);

		if (streamer_params != NULL) {
			fprintf(stream, "\nStreamer arguments:\n");
			for (i = 0; streamer_params[i].name != NULL; ++i) {
				if (streamer_params[i].flag == NULL)
					fprintf(stream, (streamer_params[i].val != 0 ? 
								"  " B "--%s=" U "value" R "\n"
								: 
								"  --%s=" U "value" R "\n" ), streamer_params[i].name);
				else
					fprintf(stream, "  --%s\n", streamer_params[i].name);
			}
		}
	}
}
