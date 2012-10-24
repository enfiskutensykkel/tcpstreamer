#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pcap.h>
#include <stdio.h>
#include "utils.h"
#include "bootstrap.h"


static int count_dupacks = 0;

static int sample_rtt = 0;



/* Send a file given to the streamer as argument using --file=filename */
int streamer(int sock, const int *run, const char **args)
{
	int bufsz = 1460;
	FILE *fp = NULL;
	char *buf = NULL;
	ssize_t len;
	pcap_t *handle = NULL;
	pkt_t pkt;
	unsigned dupacks = 0, ack_hi = 0;
	struct sockaddr_in addr;
	unsigned rtt_sample = 0;
	double rtt;


	/* Parse arguments */
	if ((fp = fopen(args[0], "r")) == NULL) {
		fprintf(stderr, "Invalid file: %s\n", args[0]);
	}
	if (args[1] != NULL && ((bufsz = atoi(args[1])) < 0 || bufsz > 2048)) {
		fprintf(stderr, "Invalid buffer size: '%s'\n", args[1]);
		return -2;
	}

	/* Create capture handle */
	if ((count_dupacks || sample_rtt) && create_handle(&handle, sock, 10) < 0) {
		fprintf(stderr, "Couldn't create handle, are you root?\n");
		return -4;
	}

	if (lookup_addr(sock, &addr, NULL) < 0) {
		destroy_handle(handle);
		return -4;
	}

	/* Allocate buffer */
	if ((buf = malloc(bufsz)) == NULL) {
		perror("malloc");
		return -4;
	}

	/* Run streamer */
	while (*run && !ferror(fp) && !feof(fp)) {

		// read from file
		len = fread(buf, sizeof(char), bufsz, fp);

		// send to receiver
		if (send(sock, buf, len, 0) < 0)
			break;

		// print packet timestamps
		while (*run && (count_dupacks || sample_rtt) && parse_segment(handle, &pkt) > 0) {
			if (sample_rtt && pkt.src.sin_addr.s_addr == addr.sin_addr.s_addr &&
					pkt.src.sin_port == addr.sin_port) {

				if (rtt_sample == 0) {
					rtt_sample = pkt.seq + pkt.len;
					rtt = pkt.ts.tv_sec * 1000.0 * 1000.0 + pkt.ts.tv_usec;
				}

			} else if (pkt.dst.sin_addr.s_addr == addr.sin_addr.s_addr &&
					pkt.dst.sin_port == addr.sin_port) {

				if (rtt_sample != 0 && pkt.ack > rtt_sample) {
					rtt = (pkt.ts.tv_sec * 1000.0 * 1000.0 + pkt.ts.tv_usec) - rtt;
					fprintf(stdout, "%lu.%06lu RTT sampled to %.2lf ms\n", pkt.ts.tv_sec, pkt.ts.tv_usec, rtt / 1000.0);
					rtt_sample = 0;
				}

				if (count_dupacks && pkt.ack > ack_hi) {
					ack_hi = pkt.ack;
					dupacks = 0;
				} else if (count_dupacks && pkt.ack == ack_hi && ++dupacks >= 3) {
					fprintf(stdout, "%lu.%06lu %d dupACKs for %u\n", pkt.ts.tv_sec, pkt.ts.tv_usec, dupacks, ack_hi);
				}
			}
		}
	}

	/* Exit gracefully */
	destroy_handle(handle);
	free(buf);
	if (fp != NULL)
		fclose(fp);

	return 0;
}

/* Register arguments for the streamer */
void streamer_create(void)
{
	register_argument("file", NULL, 1);
	register_argument("bufsz", NULL, 0);
	register_argument("show-dupacks", &count_dupacks, 1);
	register_argument("show-rtt", &sample_rtt, 1);
}
