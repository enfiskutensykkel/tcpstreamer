/* XXX Please note that these functions only work with IPv4 at the moment XXX */
#ifndef __NETUTIL__
#define __NETUTIL__

#include <arpa/inet.h>



/* Look up the hostname associated to an address
 *
 * Load hostname (with maximum len characters) with the host name associated
 * with the address provided by addr. Ensure that hostname has a size of at 
 * least INET_ADDRSTRLEN characters.
 *
 * Returns 0 on success, a negative value on failure.
 */
int lookup_name(struct sockaddr_in addr, char* hostname, int len);



/* Look up the addresses of a socket descriptor
 * 
 * Load the sockaddr_in structs with the address of each end of the socket
 * connection.
 *
 * Returns 0 on success, a negative value on failure.
 */
int lookup_addr(int socket_desc, struct sockaddr_in* local_addr, struct sockaddr_in* remote_addr);



/* Create a socket descriptor
 *
 * If hostname is NULL, try to bind to port and listen. If hostname is 
 * supplied, try to connect to the host.
 *
 * Returns the socket descriptor on success, and a negative value on failure.
 */
int create_sock(const char* hostname, const char* port);

#endif
