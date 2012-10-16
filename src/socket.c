#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>



/* Create a byte stream socket (TCP)
 *
 * If hostname is NULL, try to bind to port and listen.
 * Otherwise, try to connect to hostname.
 */
int create_socket(const char *hostname, const char *port, char *c_hostname, int len_hostname)
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

		if (c_hostname != NULL && (status = getnameinfo(ptr->ai_addr, ptr->ai_addrlen, c_hostname, len_hostname, NULL, 0, NI_NUMERICHOST))) {
			freeaddrinfo(host);
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(status));
			return -2;
		}

		freeaddrinfo(host);
	}

	return sock_desc;
}
