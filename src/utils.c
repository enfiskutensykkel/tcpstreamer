#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pcap.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "capture.h"
#include "pcap.h"



/* Look up device associated to the socket descriptor */
int lookup_dev(int sock_desc, char *dev, int len)
{
	char errstr[PCAP_ERRBUF_SIZE];
	pcap_if_t *all_devs, *ptr;
	pcap_addr_t *list;
	struct sockaddr_in addr;

	/* look up address */
	if (lookup_addr(sock_desc, &addr, NULL) != 0)
		return -2;

	/* get all devices */
	if (pcap_findalldevs(&all_devs, errstr)) {
		fprintf(stderr, "pcap_findalldevs: %s\n", errstr);
		return -2;
	}

	/* match device to address */
	for (ptr = all_devs, list = NULL; ptr != NULL; ptr = ptr->next) {
		for (list = ptr->addresses; list != NULL; list = list->next) {

			if (list->addr->sa_family == addr.sin_family) {

				if (addr.sin_addr.s_addr == ((struct sockaddr_in*) list->addr)->sin_addr.s_addr) {
					// match found
					strncpy(dev, ptr->name, len);
					dev[len-1] = '\0';
					pcap_freealldevs(all_devs);
					return 0;
				}

			}

		}
	}

	/* no match found */
	pcap_freealldevs(all_devs);
	return -1;
}



/* Look up the hostname associated to an address */
int lookup_name(struct sockaddr_in addr, char *hostname, int namelen)
{
	int status;
	socklen_t addrlen;

	addrlen = sizeof(struct sockaddr_in);
	status = getnameinfo((struct sockaddr*) &addr, addrlen, hostname, namelen, NULL, 0, NI_NUMERICHOST);

	if (status != 0) {
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(status));
		return -1;
	}

	return 0;
}



/* Look up the addresses of both sides of a socket */
int lookup_addr(int sock_desc, struct sockaddr_in *local, struct sockaddr_in *remote)
{
	socklen_t len;

	len = sizeof(struct sockaddr_in);
	if (local != NULL && getsockname(sock_desc, (struct sockaddr*) local, &len) == -1) {
		perror("getsockname");
		return -1;
	}

	len = sizeof(struct sockaddr_in);
	if (remote != NULL && getpeername(sock_desc, (struct sockaddr*) remote, &len) == -1) {
		perror("getpeername");
		return -2;
	}

	return 0;
}



/* Create a byte stream socket (TCP)
 *
 * If hostname is NULL, try to bind to port and listen.
 * Otherwise, try to connect to hostname.
 */
int create_socket(const char *hostname, const char *port)
{
	int sock_desc = -1, status;
	struct addrinfo hints, *ptr, *host;

	/* Set addrinfo hints */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = hostname == NULL ? AI_PASSIVE : 0;

	/* Get host information */
	if ((status = getaddrinfo(hostname, port, &hints, &host)) != 0) {
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -2;
	}

	if (hostname == NULL) {
		
		/* Try to bind to port */
		for (ptr = host; ptr != NULL; ptr = ptr->ai_next) {

			// try to create socket
			if ((sock_desc = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
				continue;

			// set socket reusable (so that we can reuse the port quickly)
			status = 1;
			if (setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)) != 0)
				perror("setsockopt");

			// try to bind to port
			if (bind(sock_desc, ptr->ai_addr, ptr->ai_addrlen) != -1)
				break;

		}

		if (ptr == NULL) {
			freeaddrinfo(host);
			return -1;
		}

		freeaddrinfo(host);

		if (listen(sock_desc, 10) != 0) {
			perror("listen");
			return -3;
		}

	} else {

		/* Try to connect to host through a valid interface */
		for (ptr = host; ptr != NULL; ptr = ptr->ai_next) {
			if ((sock_desc = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
				continue;
			if (connect(sock_desc, ptr->ai_addr, ptr->ai_addrlen) != -1)
				break;
		}
		
		if (ptr == NULL) {
			freeaddrinfo(host);
			return -1;
		}

		freeaddrinfo(host);
	}

	return sock_desc;
}



/* Create and compile a filter string for the pcap capture filter handle.
 * Will set up a filter that only captures packets belonging to the the
 * stream indicated by socket.
 *
 * Returns 0 on success, and a negative value on failure.
 */
static int compile_filter(const char *dev, pcap_t *handle, int sock, struct bpf_program *filter)
{
	struct sockaddr_in loc_addr, rem_addr;
	char loc_host[INET_ADDRSTRLEN],
		 rem_host[INET_ADDRSTRLEN];

	return 0;
}



/* Create a custom pcap capture filter handle and return it */
int create_handle(pcap_t** handle, int sock, int timeout)
{
	char errstr[PCAP_ERRBUF_SIZE];
	char dev[15];
	struct bpf_program filter;

	/* look up device */
	if (lookup_dev(sock, dev, sizeof(dev)))
		return -1;

	/* create a pcap capture handle */
	if ((*handle = pcap_open_live(dev, 0xffff, 1, timeout, errstr)) == NULL) {
		fprintf(stderr, "pcap_open_live: %s\n", errstr);
		return -2;
	}

	/* create and apply filter string */
	if (compile_filter(dev, *handle, sock, &filter))
		return -3;

	if (pcap_setfilter(*handle, &filter) == -1) {
		pcap_perror(*handle, "pcap_setfilter");
		pcap_freecode(&filter);
		return -4;
	}

	pcap_freecode(&filter);
	return 0;
}



/* Process a packet captured by the pcap capture filter */
int parse_segment(pcap_t *handle, pkt_t *packet)
{
	return 0;
}



/* Free up any resources associated with the pcap capture filter */
void destroy_handle(pcap_t *handle)
{
}
