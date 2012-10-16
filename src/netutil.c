#include "netutil.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>



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
int create_sock(const char *hostname, const char *port)
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
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
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
			return -2;
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
