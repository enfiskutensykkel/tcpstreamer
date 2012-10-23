#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pcap.h>
#include <getopt.h>
#include "streamer.h"
#include "utils.h"



static int count_dupacks = 0;



static int filestreamer(int sock, const state_t *run, const char **args)
{
	int bufsz = 1460;
	FILE *fp = NULL;
	char *buf = NULL;
	ssize_t len;
	pcap_t *handle = NULL;
	pkt_t pkt;
	unsigned dupacks = 0, ack_hi = 0;
	struct sockaddr_in addr;


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
	if (count_dupacks && create_handle(&handle, sock, 10) < 0) {
		fprintf(stderr, "Couldn't create handle, are you root?\n");
		return -4;
	}

	if (lookup_addr(sock, &addr, NULL) < 0) {
		destroy_handle(handle);
		return -4;
	}


	/* Allocate buffer */
	if ((buf = malloc(sizeof(char) * bufsz)) == NULL) {
		perror("malloc");
		return -3;
	}

	/* Run streamer */
	while (*run && !ferror(fp) && !feof(fp)) {

		// read from file
		len = fread(buf, sizeof(char), bufsz, fp);

		// send to receiver
		if (send(sock, buf, len, 0) < 0)
			break;

		// print packet timestamps
		while (*run && count_dupacks && parse_segment(handle, &pkt) > 0) {
			if (pkt.dst.sin_addr.s_addr == addr.sin_addr.s_addr &&
					pkt.dst.sin_port == addr.sin_port) {

				if (pkt.ack > ack_hi) {
					fprintf(stdout, "%lu.%06lu Incrementing ACK from %u to %u\n", pkt.ts.tv_sec, pkt.ts.tv_usec, ack_hi, pkt.ack);
					ack_hi = pkt.ack;
					dupacks = 0;
				} else if (pkt.ack == ack_hi && ++dupacks >= 3) {
					fprintf(stdout, "%lu.%06lu %d dupACKs for %u\n", pkt.ts.tv_sec, pkt.ts.tv_usec, dupacks, ack_hi);
				}
			}
		}
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
		{"show-dupacks", 0, &count_dupacks, 1}
	};

	if (entry_point == &filestreamer) {
		register_argument(streamer_args[0]);
		register_argument(streamer_args[1]);
		register_argument(streamer_args[2]);
	}
}


STREAMER(filestreamer, bootstrap, NULL)
