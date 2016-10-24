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

int main(int argc, char *argv[]){
  char *addressReceive = NULL;
  int port = 0;
  int opt;
	char *fileName = NULL;
	int i = 1;

	while((opt = getopt(argc, argv, "-f:"))!=-1){
		switch(opt){
		case('f'):
			fileName = optarg;
			i++;
			break;
		default:
			if(addressReceive == NULL){
				addressReceive = argv[i];
			}
			else{
				port = atoi(argv[i]);
			}
			break;
		}
		i++;
	}

  struct sockaddr_in6 *sender = (struct sockaddr_in6*)malloc(sizeof(struct sockaddr_in6));
  const char *realAddRet = real_address(addressReceive, sender);
  if(realAddRet != NULL){
    fprintf(stderr, "Error : %s\n",realAddRet);
    return -1;
  }
  int socket = create_socket(sender, port, NULL, 0);
 if (socket == -1) {
    return -1;
  }

  wait_for_client(socket);
  if(fileName != NULL){
    int fileNbr = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fileNbr == -1){
      fprintf(stderr, "ERROR %s\n",strerror(errno));
      return -1;
    }
      read_loop(socket, fileNbr);
      return 0;
  }
  //read on stdin
  else{

    read_loop(socket, fileno(stdout));
    return 0;
  }
}
