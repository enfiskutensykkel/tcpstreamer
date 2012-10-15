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


/* Create a string out of a numerical define */
#define STRINGIFY(str) #str
#define NUM_2_STR(str) STRINGIFY(str)



/* Print program usage */
static void give_usage(FILE *out, char *prog_name, int streamer_impl);



/* Create a table of streamers and their corresponding functions */
static struct tbl_ent {
	const char *name;
	streamer_t streamer;
	void (*sighndlr)(streamer_t);
	void (*init)(streamer_t);
	void (*uninit)(streamer_t);
} *streamer_tbl = NULL;



/* Streamer parameter lists */
static struct option **streamer_params = NULL;



/* Argument registration for streamers */
void register_argument(streamer_t streamer, struct option param)
{
	struct option empty = { 0, 0, 0, 0 };
	int i, j;
	
	for (i = 0; streamers[i].bootstrapper != NULL; ++i) {
		if (streamer_tbl[i].streamer == streamer)
			break;
	}
	
	if (streamers[i].bootstrapper != NULL) {
		for (j = 0; streamer_params[i][j].name != NULL; ++j);

		if ((streamer_params[i] = realloc(streamer_params[i], sizeof(struct option) * (j+2))) == NULL) {
			perror("realloc");
			exit(1);
		}

		memcpy(&streamer_params[i][j], &param, sizeof(struct option));
		memcpy(&streamer_params[i][j+1], &empty, sizeof(struct option));
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
		if (streamer_tbl[i].init != NULL)
			streamer_tbl[i].init(streamer_tbl[i].streamer);
	}


	/* Parse command line options and arguments (parameters) */
	while ((opt = getopt_long(argc, argv, ":t:p:s:", opts, &optidx)) != -1) {
		switch (opt) {

			case ':': // missing parameter value
				if (optopt == '\0')
					fprintf(stderr, "Argument %s requires a parameter\n", argv[optind-1]);
				else
					fprintf(stderr, "Option -%c requires a parameter\n", optopt);

				give_usage(stderr, argv[0], tblidx);
				goto cleanup_and_die;

			case '?': // unknown parameter
				if (optopt == '\0')
					fprintf(stderr, "Unknown argument: %s\n", argv[optind-1]);
				else
					fprintf(stderr, "Unknown option: -%c\n", optopt);

				give_usage(stderr, argv[0], tblidx);
				goto cleanup_and_die;

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

				if (streamer != NULL) {
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

				tblidx = i;
				opts = streamer_params[tblidx];
				streamer = streamer_tbl[tblidx].streamer;
			
				for (i = 0; streamer_params[tblidx][i].name != NULL; ++i)
					streamer_args[i] = NULL;

				break;

			default: // register streamer argument
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
		free(streamer_params[i]);
	}
	free(streamer_params);
	exit(0);

cleanup_and_die:
	free(streamer_tbl);
	for (i = 0; streamer_params != NULL && i < num_streamers; ++i) {
		free(streamer_params[i]);
	}
	free(streamer_params);
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
