/* XXX Please note that these functions only work with IPv4 at the moment. */
#ifndef __UTILS__
#define __UTILS__

#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pcap.h>



/* Length of an ethernet frame header
 * This is usually 14 bytes/octets long, but can be 4 bytes longer (total of 18)
 * if 802.1Q VLAN tagging is enabled.
 */
#ifndef ETH_FRAME_LEN
#define ETH_FRAME_LEN 14
#endif



/* A segment descriptor 
 * 
 * Describes properties of a byte stream segment captured by the filter.
 */
typedef struct {
	struct timeval ts;       // timestamp of when segment was caught in filter
	struct sockaddr_in dst;  // destination address and port
	struct sockaddr_in src ; // source address and port
	unsigned seq;            // segment sequence number
	unsigned ack;            // segment acknowledgement number
	unsigned len;            // payload length
	void const* payload;     // pointer to payload data
} pkt_t;



/* Look up the hostname associated to an address
 *
 * Load hostname (with maximum len characters) with the host name associated
 * with the address provided by addr. Hostname should have a length at least
 * INET_ADDRSTRLEN characters.
 *
 * Returns 0 and loads hostname on success, or a negative value on failure.
 */
int lookup_name(struct sockaddr_in addr, char* hostname, int len);



/* Look up the addresses of a socket descriptor
 * 
 * Load the sockaddr_in structs with the address of each end of the socket
 * connection.
 *
 * Returns 0 and loads the appropriate sockaddr_in structs on success, or a 
 * negative value on failure.
 */
int lookup_addr(int socket_desc, struct sockaddr_in* local_addr, struct sockaddr_in* remote_addr);



/* Look up the interface that a connection is bound through
 *
 * Load devname (with maximum len characters) with the device name of the 
 * interface associated with the socket descriptor.
 *
 * XXX Please note that on most systems, this requires superuser privileges!
 *
 * Returns 0 and loads devname on success, or a negative value on failure.
 */
int lookup_dev(int socket_desc, char* devname, int len);



/* Create a socket descriptor
 *
 * If hostname is NULL, try to bind to port and listen. If hostname is 
 * supplied, try to connect to the host.
 *
 * Returns the socket descriptor on success, or a negative value on failure.
 */
int create_socket(const char* hostname, const char* port);



/* Create a segment sniffer filter handle
 *
 * Create a byte stream segment capture filter that captures segments 
 * associated with the connection (identified by socket_desc), and load the
 * handle pointer with it.
 *
 * The timeout argument specifies how long parse_segment() should block before
 * giving up and returning 0.
 *
 * XXX Please note that on most systems, creating a capture filter requires
 *     superuser privileges.
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
