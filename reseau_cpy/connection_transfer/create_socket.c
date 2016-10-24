#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){
	int s;

	if ((s = socket(AF_INET6, SOCK_DGRAM,IPPROTO_UDP)) == -1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	if(source_addr != NULL && src_port >0){
		source_addr->sin6_port =htons((in_port_t) src_port);
		
		if( bind(s, (struct sockaddr *)source_addr, sizeof(*source_addr)) == -1){
			fprintf(stderr, "Error: %s\n", strerror(errno));
			close(s);
			printf("bah oui \n");
			return -1;
		}
	}
	else if(dest_addr != NULL && dst_port >0){
		dest_addr->sin6_port = htons((in_port_t) dst_port);
		if (connect(s, (struct sockaddr *) dest_addr, sizeof(*dest_addr)) == -1){
			fprintf(stderr, "Error: %s\n", strerror(errno));
			close(s);
			return -1;
		}
	}
	else{
		fprintf(stderr, "Error: source address and destination address both NULL");
		return -1;
	}
	return s;
}
