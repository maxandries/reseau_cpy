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

const char *real_address(const char *address, struct sockaddr_in6 *rval){
	struct addrinfo hint, *res;
	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_family = AF_INET6;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_flags = AI_PASSIVE;
	int status;
	if((status = getaddrinfo(address, NULL, &hint, &res)) != 0){
		return gai_strerror(status);
	}
	//rval = (struct sockaddr_in6 *) res->ai_addr;
	memcpy((void *)rval,(void *)res->ai_addr, sizeof(struct sockaddr_in6));
	
	return NULL;
}
