#ifndef __CAPTURE__
#define __CAPTURE__

#include <sys/time.h>


/* A segment descriptor 
 * 
 * Describes properties of a byte stream segment captured by the filter.
 */
typedef struct {
	struct timeval ts;       // timestamp of when segment was caught in filter
	struct sockaddr_in dst;  // destination address and port
	struct sockaddr_in src;  // source address and port
	unsigned seq;            // segment sequence number
	unsigned ack;            // segment acknowledgement number
	unsigned len;            // payload length
	void const* payload;     // pointer to payload data
} pkt_t;


// TODO function signatures

#endif
