#include "streamer.h"
#include "bootstrap.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>


static int run = 1;

static ssize_t simple_streamer(int sock, const char **args)
{
	int bufsz = 1460;
	FILE *fp = NULL;
	char *buf = NULL;


	/* Parse arguments */
	if (args[0] != NULL && (fp = fopen(args[0], "r")) == NULL) {	
		fprintf(stderr, "Invalid file: '%s'\n", args[0]);
		return -1;
	}
	if (args[1] != NULL && ((bufsz = atoi(args[1])) < 0 || bufsz > 2048)) {
		fprintf(stderr, "Invalid buffer size: '%s'\n", args[1]);
		return -2;
	}

	/* Allocate buffer */
	if ((buf = malloc(sizeof(char) * bufsz)) == NULL) {
		perror("malloc");
		return -3;
	}
	memset(buf, 0, sizeof(char) * bufsz);

	/* Run streamer */
	while (run && (fp == NULL || (!feof(fp) && !ferror(fp)))) {

		// read from file
		if (fp != NULL) {
			fread(buf, sizeof(char), bufsz, fp);
		}

		// send to receiver
		send(sock, buf, sizeof(char) * bufsz, 0);
	}

	if (fp != NULL)
		fclose(fp);

	return 0;
}



static void stop_streamer(streamer_t instance)
{
	assert(instance == &simple_streamer);
	run = 0;
}



static void bootstrap(streamer_t entry_point)
{
	struct option streamer_args[] = {
		{"file", 1, 0, 0},
		{"bufsz", 1, 0, 0}
	};

	assert(entry_point == &simple_streamer);
	register_argument(&simple_streamer, streamer_args[0]);
	register_argument(&simple_streamer, streamer_args[1]);
}


STREAMER(simple_streamer, stop_streamer, bootstrap)
