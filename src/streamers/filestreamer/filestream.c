#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pcap.h>
#include <getopt.h>
#include <stdio.h>
#include "utils.h"



int streamer(int sock)
{
	printf("Hello Word: %x\n", sock);
	return 0;
}
