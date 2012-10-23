#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pcap.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "debug.h"



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
		dbgerr(errstr);
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



/* Create and compile a filter string for the pcap capture filter handle.
 * Will set up a filter that only captures packets belonging to the the
 * stream indicated by socket.
 *
 * Returns 0 on success, and a negative value on failure.
 */
static int compile_filter(const char *dev, pcap_t *handle, int conn, struct bpf_program *progcode)
{
	struct sockaddr_in loc_addr, rem_addr; // the addresses and ports of this connection
	char loc_host[INET_ADDRSTRLEN],        // the hostname of "this side" of the connection
		 rem_host[INET_ADDRSTRLEN];        // the hostname of the "other side" of the connection
	unsigned short loc_port, rem_port;     // the ports of this connection
	char *filterstr = NULL;                // the filter string
	bpf_u_int32 netmask, netaddr;          // the network mask and network address
	char errstr[PCAP_ERRBUF_SIZE];         // used to store error messages from libpcap


	/* get device properties */
	if (pcap_lookupnet(dev, &netaddr, &netmask, errstr) == -1) {
		dbgerr(errstr);
		return -1;
	}


	/* get hostnames and ports */
	if (lookup_addr(conn, &loc_addr, &rem_addr) < 0)
		return -2;

	if (lookup_name(loc_addr, loc_host, sizeof(loc_host)) < 0)
		return -2;

	if (lookup_name(rem_addr, rem_host, sizeof(rem_host)) < 0)
		return -2;

	loc_port = ntohs(loc_addr.sin_port);
	rem_port = ntohs(rem_addr.sin_port);


	/* create filter string */
	if ((filterstr = malloc(512 /* ought to be enough */)) == NULL) {
		dbgerr(NULL);
		return -3;
	}
	memset(filterstr, 0, 512);
	snprintf(filterstr, 512,
			"tcp and ("
			"(dst host %s and dst port %d and src host %s and src port %d)"
			"or (src host %s and src port %d and dst host %s and dst port %d)"
			")",
			loc_host, loc_port, rem_host, rem_port, loc_host, loc_port, rem_host, rem_port);


	/* compile filter */
	if (pcap_compile(handle, progcode, filterstr, 0, netaddr) == -1) {
		pcap_perror(handle, "Unexpected error");
		free(filterstr);
		return -4;
	}

	free(filterstr);
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
		dbgerr(errstr);
		return -2;
	}

	/* create and apply filter string */
	if (compile_filter(dev, *handle, sock, &filter))
		return -3;

	if (pcap_setfilter(*handle, &filter) == -1) {
		pcap_perror(*handle, "Unexpected error");
		pcap_freecode(&filter);
		return -4;
	}

	pcap_freecode(&filter);
	return 0;
}



/* Process a packet captured by the pcap capture filter */
int parse_segment(pcap_t *handle, pkt_t *packet)
{
	struct pcap_pkthdr *hdr;
	const u_char *pkt;
	int status;
	struct sockaddr_in src_addr, dst_addr;
	uint32_t ack_no, seq_no, tcp_off, data_off;
	uint16_t win_sz, len;
   
	/* read next packet */
	status = pcap_next_ex(handle, &hdr, &pkt);
	if (status < 0) {
		pcap_perror(handle, "Unexpected error");
		return -1;
	} 

	/* load segment metadata */
	// FIXME: This test is probably unnecessary as only TCP segments should be captured by filter anyway
	if (status > 0 
			// length >= minimum length of eth frame, IP header and TCP header?
			&& (hdr->len >= ETH_FRAME_LEN+20+20) 
			// IP version 4?
			&& ((*((uint8_t*) (pkt+ETH_FRAME_LEN)) & 0xf0) >> 4) == 4
			// protocl = TCP?
			&& *((uint8_t*) (pkt+ETH_FRAME_LEN+9)) == 6
	   ) {

		/* read header data */
		tcp_off = (*((uint8_t*) pkt + ETH_FRAME_LEN) & 0x0f) * 4; // IP header size (offset to IP payload / TCP header)
		data_off = ((*((uint8_t*) (pkt + ETH_FRAME_LEN + tcp_off + 12)) & 0xf0) >> 4) * 4; // TCP header size (offset to TCP payload)

		src_addr.sin_addr = *((struct in_addr*) (pkt + ETH_FRAME_LEN + 12)); // source address
		src_addr.sin_port = *((uint16_t*) (pkt + ETH_FRAME_LEN + tcp_off)); // source port
		dst_addr.sin_addr = *((struct in_addr*) (pkt + ETH_FRAME_LEN + 16)); // destination address
		dst_addr.sin_port = *((uint16_t*) (pkt + ETH_FRAME_LEN + tcp_off + 2)); // destination port

		seq_no = ntohl(*((uint32_t*) (pkt + ETH_FRAME_LEN + tcp_off + 4))); // sequence number
		ack_no = ntohl(*((uint32_t*) (pkt + ETH_FRAME_LEN + tcp_off + 8))); // acknowledgement number

		win_sz = ntohl(*((uint16_t*) (pkt + ETH_FRAME_LEN + tcp_off + 14))); // window size

		len = ntohs(*((uint16_t*) (pkt + ETH_FRAME_LEN + 2))) - tcp_off - data_off; // payload length

		/* discard if SYN or FIN flag set (connection handshake/teardown) */
		if ((*((uint8_t*) (pkt + ETH_FRAME_LEN + tcp_off + 13)) & 0x02))
			return 0;
		

		/* load struct with header data */
		packet->ts = hdr->ts;
		packet->dst = dst_addr;
		packet->src = src_addr;
		packet->seq = seq_no;
		packet->ack = ack_no;
		packet->len = len;
		packet->payload = (void const*) ((uint8_t*) (pkt + ETH_FRAME_LEN + tcp_off + data_off)); // FIXME: Verify that this is correct

		return status;
	} 

	return 0;
}



/* Free up any resources associated with the pcap capture filter */
void destroy_handle(pcap_t *handle)
{
	if (handle != NULL)
		pcap_close(handle);
}
