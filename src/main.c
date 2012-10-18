#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <assert.h>
#include "netutils.h"
#include "streamerctl.h"
#include "bootstrap.h"



/* Table of streamers and their corresponding functions */
static tblent_t *streamer_tbl = NULL;

/* Streamer parameter lists */
static struct option **streamer_params = NULL;

/* Current streamer table index */
static int streamer_idx = -1;

/* Are we running? */
static state_t streamer_run = STOP;





/* Run receiver */
extern void receiver(int sock_desc, state_t *cond);


/* Signal handler to catch user interruption */
static void handle_signal()
{
	streamer_run = STOP;
}


/* Print program usage */
static void give_usage(FILE *out, char *prog_name, int streamer_impl);





/* Parse command line arguments and start program */
int main(int argc, char **argv)
{
#define STRINGIFY(str) #str
#define NUM_2_STR(str) STRINGIFY(str)

	int i,                             // used to iterate stuff
		num_streamers,                 // total number of streamers
		socket_desc = -1;              // socket descriptor
	char hostname[INET_ADDRSTRLEN],    // the canonical hostname
		 *port = NUM_2_STR(DEF_PORT),  // port number (as string)
		 *host = NULL,                 // hostname given as argument
		 *sptr = NULL;                 // used to verified that a proper number is given to strtoul
	char const **streamer_args = NULL; // arguments passed on to the streamer
	unsigned duration = DEF_DUR;       // streamer duration
	struct option *opts = NULL;        // used for giving opts to getopt_long when a streamer is selected
	struct sockaddr_in addr;           // used just for host pretty print
	

	/* Initialize streamer table */
	num_streamers = streamer_tbl_create(&streamer_tbl);
	assert(num_streamers >= 0);


	/* Set up the argument registration */
	if ((streamer_params = malloc(sizeof(struct option*) * num_streamers)) == NULL) {
		perror("malloc");
		goto cleanup_and_die;
	}
	for (i = 0; i < num_streamers; ++i) {
		if ((streamer_params[i] = malloc(sizeof(struct option))) == NULL) {
			perror("malloc");
			goto cleanup_and_die;
		}
		memset(&streamer_params[i][0], 0, sizeof(struct option));
	}
	

	/* Initialize streamers */
	for (i = 0; i < num_streamers; ++i) {
		if (streamer_tbl[i].init != NULL) {
			streamer_idx = i;
			streamer_tbl[i].init(streamer_tbl[i].streamer);
		}
	}
	streamer_idx = -1;


	/* Parse command line options and arguments (parameters) */
	int opt, optidx, help = 0;
	while ((opt = getopt_long(argc, argv, ":ht:p:s:", opts, &optidx)) != -1) {
		switch (opt) {

			case ':': // missing parameter value
				if (optopt == '\0')
					fprintf(stderr, "Argument %s requires a value\n", argv[optind-1]);
				else
					fprintf(stderr, "Option -%c requires a parameter\n", optopt);

				give_usage(stderr, argv[0], streamer_idx);
				goto cleanup_and_die;

			case '?': // unknown parameter
				if (optopt == '\0')
					fprintf(stderr, "Unknown argument: %s\n", argv[optind-1]);
				else
					fprintf(stderr, "Unknown option: -%c\n", optopt);

				give_usage(stderr, argv[0], streamer_idx);
				goto cleanup_and_die;

			case 'h': // show help
				help = 1;
				break;

			case 'p': // port
				sptr = NULL;
				if (strtoul(optarg, &sptr, 0) > 0xffff || sptr == NULL || *sptr != '\0') {
					fprintf(stderr, "Option -p requires a valid port number\n");
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}
				port = optarg;
				break;

			case 't': // duration
				sptr = NULL;
				duration = strtoul(optarg, &sptr, 10);
				if (sptr == NULL || *sptr != '\0') {
					fprintf(stderr, "Option -t requires a valid number of seconds\n");
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}
				break;

			case 's': // select streamer

				if (streamer_idx != -1) {
					fprintf(stderr, "Streamer is already selected\n");
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}

				for (i = 0; i < num_streamers; ++i)
					if (!strcmp(optarg, streamer_tbl[i].name))
						break;

				if (i == num_streamers) {
					fprintf(stderr, "Streamer '%s' is not found\n", optarg);
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}

				streamer_idx = i;
				opts = streamer_params[streamer_idx];

				for (i = 0; streamer_params[streamer_idx][i].name != NULL; ++i);
				streamer_args = malloc(sizeof(char**) * i);
			
				for (i = 0; streamer_params[streamer_idx][i].name != NULL; ++i)
					streamer_args[i] = NULL;

				break;

			default: // register streamer argument
				if (streamer_idx != -1)
					streamer_args[optidx] = optarg;

				break;
		}
	}
	if (help) {
		give_usage(stdout, argv[0], streamer_idx);
		goto cleanup_and_die;
	}


	/* Create signal handler to deal with interruption */
	assert(signal(SIGINT, (void (*)(int)) &handle_signal) != SIG_ERR);


	/* Create socket descriptor and start instance */
	socket_desc = -1; streamer_run = RUN;
	if (streamer_idx == -1) {

		fprintf(stdout, "No streamer selected, starting receiver.\n");
		if ((socket_desc = create_socket(NULL, port)) < 0) {
			fprintf(stderr, "Unable to bind to port %s\n", port);
			goto cleanup_and_die;
		}
		fprintf(stdout, "Listening to port %s\n", port);

		/* Start receiver instance */
		receiver(socket_desc, &streamer_run);

	} else if (argc - optind > 0) {
		
		host = argv[optind];
		fprintf(stdout, "Streamer '%s' selected.\n", streamer_tbl[streamer_idx].name);
		if ((socket_desc = create_socket(host, port)) < 0) {
			fprintf(stderr, "Unable to connect to '%s'\n", host);
			goto cleanup_and_die;
		}
		
		lookup_addr(socket_desc, NULL, &addr);
		lookup_name(addr, hostname, sizeof(hostname));

		fprintf(stdout, "Successfully connected to %s\n", hostname);

		/* Start streamer */
		streamer(&streamer_tbl[streamer_idx], duration, socket_desc, &streamer_run, streamer_args);


	} else {
		fprintf(stderr, "Missing host argument\n");
		give_usage(stderr, argv[0], streamer_idx);
		goto cleanup_and_die;
	}
	

	/* Uninitialize streamers */
	for (i = 0; i < num_streamers; ++i) {
		if (streamer_tbl[i].uninit != NULL)
			streamer_tbl[i].uninit(streamer_tbl[i].streamer);
	}


	/* Clean up */
	free(streamer_args);
	close(socket_desc);
	streamer_tbl_destroy(streamer_tbl);

	for (i = 0; i < num_streamers; ++i) {
		free(streamer_params[i]);
	}
	free(streamer_params);

	exit(0);


cleanup_and_die:
	free(streamer_args);

	if (socket_desc >= 0)
		close(socket_desc);
	free(streamer_tbl);
	for (i = 0; streamer_params != NULL && i < num_streamers; ++i) {
		free(streamer_params[i]);
	}
	free(streamer_params);
	exit(1);
}



/* Argument registration for streamers */
int register_argument(struct option argument)
{
	struct option empty = { 0, 0, 0, 0 };
	int i;

	if (streamer_idx == -1)
		return -1;
	
	for (i = 0; streamer_params[streamer_idx][i].name != NULL; ++i)
		if (strcmp(argument.name, streamer_params[streamer_idx][i].name) == 0)
			return -1;

	if ((streamer_params[streamer_idx] = realloc(streamer_params[streamer_idx], sizeof(struct option) * (i+2))) == NULL) {
		perror("realloc");
		return -1;
	}

	memcpy(&streamer_params[streamer_idx][i], &argument, sizeof(struct option));
	memcpy(&streamer_params[streamer_idx][i+1], &empty, sizeof(struct option));

	return i;
}



/* Print program usage */
static void give_usage(FILE *fp, char *name, int streamer)
{
	int i, n;
	if (streamer < 0) {
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

		fprintf(fp, "\nAvailable streamers:\n  ");
		for (i = 0, n = 0; streamer_tbl[i].streamer != NULL; n += 2 + strlen(streamer_tbl[i++].name)) {

			if (n + 2 + strlen(streamer_tbl[i].name) > 80) 
				n = 0;

			if (i > 0 && n == 0)
				fprintf(fp, ",\n  ");
			else if (i > 0)
				fprintf(fp, ", ");

			fprintf(fp, "%s", streamer_tbl[i].name);
		}
		fprintf(fp, "\n");

	} else {
		fprintf(fp, "Usage: %s " B "-s %s" R " " U "arguments" R "... " U "host" R "\n"
				"\nArguments:\n"
				,
				name, streamer_tbl[streamer].name);

		for (i = 0; streamer_params[streamer][i].name != NULL; ++i) {
			fprintf(fp, "  --%s%s\n", streamer_params[streamer][i].name, (streamer_params[streamer][i].has_arg > 0 ? "=" U "value" R : ""));
		}
	}
}
