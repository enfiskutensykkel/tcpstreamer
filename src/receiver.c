#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "netutils.h"



static void reject_connection(int listen_sock)
{
	int sock_fd;
	sock_fd = accept(listen_sock, NULL, NULL);
	close(sock_fd);
}



static int accept_connection(int listen_sock, struct sockaddr_in *addr, int *sock)
{
	int recv_sock, status;
	socklen_t len = sizeof(struct sockaddr_in);

	/* accept connection */
	recv_sock = accept(listen_sock, (struct sockaddr*) addr, &len);
	if (recv_sock == -1) {
		perror("accept");
		return -1;
	}

	/* set socket to non-blocking */
	status = fcntl(recv_sock, F_GETFL, 0);
	fcntl(recv_sock, F_SETFL, status | O_NONBLOCK);

	/* return socket descriptor */
	*sock = recv_sock;
	return *sock;
}



void* receiver(void *argv)
{
	struct conn {
		struct sockaddr_in addr; // address of the remote side of the connection
		int                sock; // conn socket descriptor
		ssize_t            rcvd; // number of bytes received from connection
		struct conn*       next; // next connection
		struct conn*       prev; // previous connection
	} *conns = NULL, *ptr, *tmp;

	char name[INET_ADDRSTRLEN];
	void *buf = NULL;
	ssize_t rcvd, tot_rcvd;
	int hi_sock, num_active;
	fd_set socks, active;
	int listen_sock;
	int *run;
	struct timeval wait = {0, 0};

	/* Get thread arguments */
	listen_sock = *((int*) argv);
	run = *((int**) ((int*) argv+1));

	/* Allocate buffer */
	if ((buf = malloc(sizeof(char) * 1460)) == NULL) {
		perror("malloc");
		pthread_exit(NULL);
	}

	/* Initialize descriptor set */
	FD_ZERO(&socks);
	FD_SET(listen_sock, &socks);
	hi_sock = listen_sock;
	
	while (*run == 1) {

		active = socks;
		if ((num_active = select(hi_sock + 1, &active, NULL, NULL, &wait)) == -1) 
			break;

		/* Accept incomming connection */
		if (FD_ISSET(listen_sock, &active)) {
			
			// allocate new connection
			for (ptr = conns; ptr != NULL && ptr->next != NULL; ptr = ptr->next);
			if (ptr == NULL) {
				if ((conns = malloc(sizeof(struct conn))) == NULL) {
					perror("malloc");
					reject_connection(listen_sock);
					break;
				}

				ptr = conns;
				ptr->next = NULL;
				ptr->prev = NULL;

			} else {
				if ((ptr->next = malloc(sizeof(struct conn))) == NULL) {
					perror("malloc");
					reject_connection(listen_sock);
					break;
				}

				ptr->next->prev = ptr;
				ptr->next->next = NULL;
				ptr = ptr->next;
			}

			// set up new connection
			if (accept_connection(listen_sock, &(ptr->addr), &(ptr->sock)) < 0)
				break; // something is wrong
			
			ptr->rcvd = 0;

			lookup_name(ptr->addr, name, sizeof(name));
			fprintf(stdout, "Accepted connection from %s\n", name);

			// update descriptor set
			FD_SET(ptr->sock, &socks);
			hi_sock = ptr->sock > hi_sock ? ptr->sock : hi_sock;
			--num_active;
		}


		/* Read data from the connections */
		for (ptr = conns; num_active > 0 && ptr != NULL; ptr = ptr->next) {
			if (FD_ISSET(ptr->sock, &active)) {

				tot_rcvd = 0;
				while ((rcvd = read(ptr->sock, buf, sizeof(char) * 1460)) > 0) {
					tot_rcvd += rcvd;
					ptr->rcvd += rcvd;
				}

				lookup_name(ptr->addr, name, sizeof(name));
				fprintf(stdout, "Received %ld bytes from %s\n", tot_rcvd, name);
				
				if (rcvd == 0 || (rcvd < 0 && errno != EAGAIN)) {
					fprintf(stdout, "Closing connection from %s\n", name);

					close(ptr->sock);
					FD_CLR(ptr->sock, &socks);
					
					if (ptr->prev != NULL)
						ptr->prev->next = ptr->next;

					if (ptr->next != NULL)
						ptr->next->prev = ptr->prev;


					if (ptr->prev != NULL) {
						tmp = ptr;
						ptr = ptr->prev;
						free(tmp);
					} else {
						free(conns);
						conns = NULL;
						break;
					}
				}

				--num_active;
			}
		}
	}

	/* Free resources */
	free(buf);
	for (ptr = conns; ptr != NULL; ptr = ptr->next) {
		close(ptr->sock);
		FD_CLR(ptr->sock, &socks);
	}
	free(conns);
	FD_ZERO(&socks);

	pthread_exit(NULL);
}
