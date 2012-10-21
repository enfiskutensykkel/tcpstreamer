#include "streamer.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <pcap.h>


static int simple_streamer(int sock, const state_t *cond, const char **args)
{
	int bufsz = 1460;
	FILE *fp = NULL;
	char *buf = NULL;
	ssize_t len;
	pcap_t *handle;
	pkt_t pkt;


	/* Parse arguments */
	if (args[0] == NULL) {
		fprintf(stderr, "No file given\n");
		return -1;
	} else if ((fp = fopen(args[0], "r")) == NULL) {	
		fprintf(stderr, "Invalid file: '%s'\n", args[0]);
		return -1;
	}
	if (args[1] != NULL && ((bufsz = atoi(args[1])) < 0 || bufsz > 2048)) {
		fprintf(stderr, "Invalid buffer size: '%s'\n", args[1]);
		return -2;
	}

	/* Create capture handle */
	if (create_handle(&handle, sock, 100) < 0) {
		fprintf(stderr, "Couldn't create handle, are you root?\n");
		return -4;
	}


	/* Allocate buffer */
	if ((buf = malloc(sizeof(char) * bufsz)) == NULL) {
		perror("malloc");
		return -3;
	}
	memset(buf, 0, sizeof(char) * bufsz);

	/* Run streamer */
	while (*cond == RUN && !feof(fp) && !ferror(fp)) {

		// read from file
		len = fread(buf, sizeof(char), bufsz, fp);

		// send to receiver
		send(sock, buf, len, 0);

		// print packet timestamps
		while (parse_segment(handle, &pkt) > 0)
			fprintf(stdout, "%lu.%lu\n", pkt.ts.tv_sec, pkt.ts.tv_usec);
	}


	destroy_handle(handle);
	free(buf);
	if (fp != NULL)
		fclose(fp);

	return 0;
}



static void bootstrap(streamer_t entry_point)
{
	struct option streamer_args[] = {
		{"file", 1, 0, 0},
		{"bufsz", 1, 0, 0},
		{"print", 0, 0, 0}
	};

	assert(entry_point == &simple_streamer);
	register_argument(streamer_args[0]);
	register_argument(streamer_args[1]);
	register_argument(streamer_args[2]);
}


STREAMER(simple_streamer, bootstrap, NULL)
