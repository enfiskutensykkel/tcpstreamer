#ifndef __CAPTURE__
#define __CAPTURE__

#include <sys/time.h>


/* A segment descriptor 
 * 
 * Describes properties of a byte stream segment captured by the filter.
 */
typedef struct {
	struct sockaddr_in dst;
	struct sockaddr_in src;
	unsigned seq;
	unsigned ack;
	unsigned len;
	void const* payload;
} pkt_t;

typedef void (*callback_t)(struct timeval, pkt_t);

int register_callback(callback_t parser_func);

#endif
