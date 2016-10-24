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


int  wait_for_client(int sfd){
	struct sockaddr_in6 addr;
	char *buf[1024];
	memset(&addr, 0, sizeof(struct sockaddr_in6));
	socklen_t length = sizeof(struct sockaddr_in6);
	recvfrom(sfd, buf, sizeof(buf), MSG_PEEK,(struct sockaddr *) &addr, &length);
	connect(sfd,(struct sockaddr *) &addr, length);
	return 0;
}