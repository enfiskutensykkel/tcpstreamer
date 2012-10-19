/* XXX Please note that these functions only work with IPv4 at the moment XXX */
#ifndef __PCAP__
#define __PCAP__

#include "capture.h"
#include <pcap.h>



/* Create a segment sniffer filter handle
 *
 * Create a byte stream segment capture filter that captures segments 
 * associated with the connection (identified by socket_desc), and load the
 * handle pointer with it.
 *
 * The timeout argument specifies how long parse_segment() should block before
 * giving up and returning 0.
 *
 * Returns 0 and loads handle on success, or a negative value on failure.
 */
int create_handle(pcap_t** handle, int socket_desc, int timeout);



/* Parse a segment captured by the segment sniffer filter
 *
 * Parse a segment from the capture filter and load seg with the appropriate
 * data.
 *
 * Returns 1 if successful and loads seg, returns 0 if no segment is captured
 * within a timeout period, see create_handle(), or a negative value on 
 * failure.
 */
int parse_segment(pcap_t* handle, pkt_t* seg);



/* Free up the resources associated with the segment sniffer handle. */
void destroy_handle(pcap_t* handle);


#endif
