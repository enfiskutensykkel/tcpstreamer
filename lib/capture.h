#ifndef __CAPTURE__
#define __CAPTURE__

#include <sys/time.h>
#include <sys/socket.h>


/* A segment descriptor 
 * 
 * Describes properties of a byte stream segment captured by the filter.
 */
typedef struct {
	struct timeval ts;       // timestamp of when segment was caught in filter
	struct sockaddr dst;     // destination address and port
	struct sockaddr src;     // source address and port
	unsigned seq;            // segment sequence number
	unsigned ack;            // segment acknowledgement number
	unsigned len;            // payload length
	void const* payload;     // pointer to payload data
} pkt_t;

typedef void (*callback_t)(pkt_t const*);

int register_callback(callback_t parser);

#endif
