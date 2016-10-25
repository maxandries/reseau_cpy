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
#include <zlib.h>
#include <fcntl.h>
#include "./packet/packet_interface.h"
#include "./connection_transfer/create_socket.h"
#include "./connection_transfer/write_loop.h"
#include "./connection_transfer/real_address.h"
#include "./connection_transfer/wait_for_client.h"

int main(int argc,char *const *argv){
	int opt;
	char *fileName = NULL;
	char *address = NULL;
	int port = 0;
	int i = 1;
	while((opt = getopt(argc, argv, "-f:"))!=-1){
		switch(opt){
		case('f'):
			printf("optarg %s\n", optarg);
			fileName = optarg;
			i++;
			break;
		default:
			if(address == NULL){
				
				address = argv[i];
			}
			else{
				port = atoi(argv[i]);
				
			}
			break;
		}
		i++;
	}
	struct sockaddr_in6 *IPaddress = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
	const char *ret = real_address((const char*)address, IPaddress);
	if(ret!=NULL){
		fprintf(stderr, "ERROR: %s\n", ret);
		return -1;
	}

	int s = create_socket(NULL, 0, IPaddress, port);
	if(s == -1){
		return -1;
	}
	if(fileName != NULL){
		int fileNbr = open(fileName, O_RDONLY);
		if(fileNbr == -1){
			fprintf(stderr, "ERROR %s\n",strerror(errno));
			return -1;
		}
			write_loop(s, fileNbr);
			return 0;
	}
	//read on stdin
	else{
		write_loop(s, fileno(stdin));
		free(IPaddress);
		return 0;
	}
}
