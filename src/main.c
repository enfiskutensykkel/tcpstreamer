#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include "netutils.h"
#include "table.h"
#include "bootstrap.h"



/* Table of streamers and their corresponding functions */
static tblent_t *streamer_tbl = NULL;

/* Streamer parameter lists */
static struct option **streamer_params = NULL;

/* Current streamer table index */
static int streamer_idx = -1;



/* Stream receiver */
extern void receiver(int socket_desc);


/* Signal handler to catch user interruption */
static void handle_signal()
{
	if (streamer_idx != -1) {
		streamer_tbl[streamer_idx].sighndlr(streamer_tbl[streamer_idx].streamer); 
	} else
		receiver(-1); 
}


/* Print program usage */
static void give_usage(FILE *out, char *prog_name, int streamer_impl);





/* Parse command line arguments and start program */
int main(int argc, char **argv)
{
#define STRINGIFY(str) #str
#define NUM_2_STR(str) STRINGIFY(str)

	int i, 
		num_streamers,                 // total number of streamers
		socket_desc = -1;              // socket descriptor
	char hostname[INET_ADDRSTRLEN],    // the canonical hostname
		 *port = NUM_2_STR(DEF_PORT),  // port number (as string)
		 *host = NULL,                 // hostname given as argument
		 *sptr = NULL;
	char const *streamer_args[argc];   // arguments passed on to the streamer
	unsigned duration = DEF_DUR;       // streamer duration
	struct option *opts = NULL;
	struct sockaddr_in addr;
	

	/* Initialize streamer table */
	num_streamers = streamer_tbl_create(&streamer_tbl);
	if (num_streamers < 0) {
		perror("streamer_tbl_init");
		exit(1);
	}


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
					fprintf(stderr, "Argument %s requires a parameter\n", argv[optind-1]);
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
	if (signal(SIGINT, (void (*)(int)) &handle_signal) == SIG_ERR) {
		perror("signal");
		goto cleanup_and_die;
	}


	/* Create socket descriptor and start instance */
	socket_desc = -1;
	if (streamer_idx == -1) {

		fprintf(stdout, "No streamer selected, starting receiver.\n");
		if ((socket_desc = create_sock(NULL, port)) < 0) {
			fprintf(stderr, "Unable to bind to port %s\n", port);
			goto cleanup_and_die;
		}
		fprintf(stdout, "Listening to port %s\n", port);

		/* Start receiver */
		receiver(socket_desc);

	} else if (argc - optind > 0) {
		
		host = argv[optind];
		fprintf(stdout, "Streamer '%s' selected, connecting...\n", streamer_tbl[streamer_idx].name);
		if ((socket_desc = create_sock(host, port)) < 0) {
			fprintf(stderr, "Unable to connect to '%s'\n", host);
			goto cleanup_and_die;
		}
		
		lookup_addr(socket_desc, NULL, &addr);
		lookup_name(addr, hostname, sizeof(hostname));

		fprintf(stdout, "Successfully connected to %s\n", hostname);

		/* Start streamer */
		if (streamer_tbl[streamer_idx].streamer(socket_desc, streamer_args) < 0) {
			goto cleanup_and_die;
		}

	} else {
		fprintf(stderr, "Streamer '%s' selected, but no host is given\n", streamer_tbl[streamer_idx].name);
		give_usage(stderr, argv[0], streamer_idx);
		goto cleanup_and_die;
	}
	

	/* Uninitialize streamers */
	for (i = 0; i < num_streamers; ++i) {
		if (streamer_tbl[i].uninit != NULL)
			streamer_tbl[i].uninit(streamer_tbl[i].streamer);
	}


	/* Clean up */
	close(socket_desc);

	streamer_tbl_destroy(streamer_tbl);

	for (i = 0; i < num_streamers; ++i) {
		free(streamer_params[i]);
	}
	free(streamer_params);

	exit(0);


cleanup_and_die:
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
				"Usage: %s [-p <port>] [-t <duration>] -s <streamer> [<args>...] <host>\n"
				"   or: %s [-p <port>]\n\n"
				"  -p\tUse <port> instead of default port.\n"
				"  -s\tStart streamer instance.\n"
				"  -t\tRun streamer for <duration> seconds.\n"
				,
				name, name);

		fprintf(fp, "\nAvailable streamer implementations are:\n  ");
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
		fprintf(fp, "Usage: %s [-p <port>] [-t <duration>] -s %s [<args>...] <host>\n"
				"  -p\tUse <port> instead of default port.\n"
				"  -t\tRun streamer for <duration> seconds.\n\n"
				"Where <args> is one of the following:\n  "
				,
				name, streamer_tbl[streamer].name);

		for (i = 0, n = 0; streamer_params[streamer][i].name != NULL; n += 4 + strlen(streamer_params[streamer][i++].name)) {

			if (n + 4 + strlen(streamer_params[streamer][i].name) > 80) 
				n = 0;

			if (i > 0 && n == 0)
				fprintf(fp, ",\n  ");
			else if (i > 0)
				fprintf(fp, ", ");

			fprintf(fp, "--%s", streamer_params[streamer][i].name);
		}
		fprintf(fp, "\n");
	}
}
