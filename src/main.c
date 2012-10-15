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
#include "bootstrap.h"



/* Print program usage */
static void give_usage(FILE *out, char *prog_name, int streamer_impl);


/* Create a string out of a numerical define */
#define STRINGIFY(str) #str
#define NUM_2_STR(str) STRINGIFY(str)



/* Create a table of streamers and their corresponding functions */
static struct tbl_ent {
	const char *name;
	streamer_t streamer;
	void (*sighndlr)(streamer_t);
	void (*init)(streamer_t);
	void (*uninit)(streamer_t);
} *streamer_tbl = NULL;



/* Streamer option lists */
static struct option **streamer_opts = NULL;

/* Option registration for streamers */
void register_option(streamer_t streamer, struct option opts)
{
	struct option empty = { 0, 0, 0, 0 };
	int i, j;
	
	for (i = 0; streamers[i].bootstrapper != NULL; ++i) {
		if (streamer_tbl[i].streamer == streamer)
			break;
	}
	
	if (streamers[i].bootstrapper != NULL) {
		for (j = 0; streamer_opts[i][j].name != NULL; ++j);

		if ((streamer_opts[i] = realloc(streamer_opts[i], sizeof(struct option) * (j+2))) == NULL) {
			perror("realloc");
			exit(1);
		}

		memcpy(&streamer_opts[i][j], &opts, sizeof(struct option));
		memcpy(&streamer_opts[i][j+1], &empty, sizeof(struct option));
	}
}




/* Parse command line arguments and start program */
int main(int argc, char **argv)
{
	int i, num_streamers, opt, tblidx, optidx;
	char *port = NUM_2_STR(DEF_PORT), *sptr = NULL;
	unsigned duration = DEF_DUR;
	streamer_t streamer = NULL;
	char const *streamer_args[argc];
	struct option *opts = NULL;
	

	/* Initialize streamer table */
	for (num_streamers = 0; streamers[num_streamers].bootstrapper != NULL; ++num_streamers);
	if ((streamer_tbl = malloc(sizeof(struct tbl_ent) * num_streamers)) == NULL) {
		perror("malloc");
		exit(1);
	}
	for (i = 0; i < num_streamers; ++i) {
		streamer_tbl[i].name = streamers[i].name;
		streamers[i].bootstrapper( &(streamer_tbl[i].streamer), &(streamer_tbl[i].sighndlr), &(streamer_tbl[i].init), &(streamer_tbl[i].uninit) );
	}


	/* Set up the option registration */
	if ((streamer_opts = malloc(sizeof(struct option*) * num_streamers)) == NULL) {
		perror("malloc");
		goto cleanup_and_die;
	}
	for (i = 0; i < num_streamers; ++i) {
		if ((streamer_opts[i] = malloc(sizeof(struct option))) == NULL) {
			perror("malloc");
			goto cleanup_and_die;
		}
		memset(&streamer_opts[i][0], 0, sizeof(struct option));
	}
	

	/* Initialize streamers */
	for (i = 0; i < num_streamers; ++i) {
		if (streamer_tbl[i].init != NULL)
			streamer_tbl[i].init(streamer_tbl[i].streamer);
	}


	/* Parse command line arguments */
	while ((opt = getopt_long(argc, argv, ":t:p:s:", opts, &optidx)) != -1) {
		switch (opt) {

			case 'p':
				sptr = NULL;
				if (strtoul(optarg, &sptr, 0) > 0xffff || sptr == NULL || *sptr != '\0') {
					fprintf(stderr, "Option -p requires a valid port number\n");
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}
				port = optarg;
				break;

			case 't':
				sptr = NULL;
				duration = strtoul(optarg, &sptr, 10);
				if (sptr == NULL || *sptr != '\0') {
					fprintf(stderr, "Option -t requires a valid number of seconds\n");
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}
				break;

			case 's':
				if (streamer != NULL) {
					fprintf(stderr, "Option -s is already set\n");
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}

				for (i = 0; i < num_streamers; ++i)
					if (!strcmp(optarg, streamer_tbl[i].name))
						break;

				if (i == num_streamers) {
					fprintf(stderr, "Streamer %s is not found\n", optarg);
					give_usage(stderr, argv[0], -1);
					goto cleanup_and_die;
				}

				tblidx = i;
				opts = streamer_opts[tblidx];
				streamer = streamer_tbl[tblidx].streamer;
			
				for (i = 0; streamer_opts[tblidx][i].name != NULL; ++i)
					streamer_args[i] = NULL;

				break;

			case ':':
				if (optopt == '\0')
					fprintf(stderr, "Argument %s requires a parameter\n", argv[optind-1]);
				else
					fprintf(stderr, "Option -%c requires a parameter\n", optopt);

				give_usage(stderr, argv[0], tblidx);
				goto cleanup_and_die;

			case '?':
				if (optopt == '\0')
					fprintf(stderr, "Unknown argument: %s\n", argv[optind-1]);
				else
					fprintf(stderr, "Unknown option: -%c\n", optopt);

				give_usage(stderr, argv[0], tblidx);
				goto cleanup_and_die;

			default:
				/* Parse arguments to the streamer */
				if (streamer != NULL)
					streamer_args[optidx] = optarg;
				break;
		}
	}

	
	// XXX
	if (streamer)
		streamer(0xdeadbeef, streamer_args);


	/* Uninitialize streamers */
	for (i = 0; i < num_streamers; ++i) {
		if (streamer_tbl[i].uninit != NULL)
			streamer_tbl[i].uninit(streamer_tbl[i].streamer);
	}

	/* Clean up */
	free(streamer_tbl);
	for (i = 0; i < num_streamers; ++i) {
		free(streamer_opts[i]);
	}
	free(streamer_opts);

	exit(0);

cleanup_and_die:

	free(streamer_tbl);
	for (i = 0; streamer_opts != NULL && i < num_streamers; ++i) {
		free(streamer_opts[i]);
	}
	free(streamer_opts);
	exit(1);
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
		for (i = 0, n = 0; streamers[i].bootstrapper != NULL; n += 2 + strlen(streamers[i++].name)) {

			if (n + 2 + strlen(streamers[i].name) > 80) 
				n = 0;

			if (i > 0 && n == 0)
				fprintf(fp, ",\n  ");
			else if (i > 0)
				fprintf(fp, ", ");

			fprintf(fp, "%s", streamers[i].name);
		}
		fprintf(fp, "\n");

	} else {
		fprintf(fp, "Usage: %s [-p <port>] [-t <duration>] -s %s [<args>...] <host>\n"
				"  -p\tUse <port> instead of default port.\n"
				"  -t\tRun streamer for <duration> seconds.\n\n"
				"Where <args> is one of the following:\n  "
				,
				name, streamers[streamer].name);

		for (i = 0, n = 0; streamer_opts[streamer][i].name != NULL; n += 4 + strlen(streamer_opts[streamer][i++].name)) {

			if (n + 4 + strlen(streamer_opts[streamer][i].name) > 80) 
				n = 0;

			if (i > 0 && n == 0)
				fprintf(fp, ",\n  ");
			else if (i > 0)
				fprintf(fp, ", ");

			fprintf(fp, "--%s", streamer_opts[streamer][i].name);
		}
		fprintf(fp, "\n");

	}
}
