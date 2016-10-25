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
#include <time.h>
#include "../packet/packet_interface.h"
#include "write_loop.h"

void delWindow(pkt_t *buffer[], uint8_t seq, int *j, int *sentPackets){
	for(int i = 0; i<MAX_WINDOW_SIZE; i++){
		if (buffer[i] != NULL) {

			if(pkt_get_seqnum(buffer[i]) <= seq && (seq - pkt_get_seqnum(buffer[i]))<= MAX_WINDOW_SIZE){
				printf("delete seqnum%d\n", pkt_get_seqnum(buffer[i]));
				pkt_del(*(buffer+i));
				buffer[i] = NULL;
				(*j)++;
				(*sentPackets)--;
			}
			else if(pkt_get_seqnum(buffer[i]) > seq && (seq+255)-pkt_get_seqnum(buffer[i]) <= MAX_WINDOW_SIZE){
				printf("delete seqnum%d\n", pkt_get_seqnum(buffer[i]));
				pkt_del(*(buffer+i));
				buffer[i] = NULL;
				(*j)++;
				(*sentPackets)--;
			}
		}
	}
}
void create_ack(pkt_t *ack,pkt_t *received){
		pkt_set_type(ack,PTYPE_ACK);
		pkt_set_seqnum(ack,(pkt_get_seqnum(received)%256)+1);
		pkt_set_timestamp(ack,pkt_get_timestamp(received));
	}

//function tu send data from sender to receiver
void write_loop(const int socket, const int in){

	fd_set read_fds;
	fd_set write_fds;

	char buf[524]; //buf where the pkt is Encoded
	char temp[512]; //buf where the read data is stored;

	pkt_t *window[MAX_WINDOW_SIZE];//buffer of struct waiting for acknowledgement

	for(int i = 0; i<MAX_WINDOW_SIZE;i++){
		window[i] = NULL;
	}

	int i = 31;//i is the number of empty place in the array window
	int a = -1; //is neg to enter the while loop then represent the numbers of bytes read/write
	int seqnum = 0;
	int sentPacket = 0;
	int lastIsReceived = 0;
	int lastIsSend = 0;
	int encodeLast = 0;

	uint8_t seqnumLast;

	size_t bufSize = 524; //max size of the buffer to encode/decode 12(struct)+512ll (MAX_PAYLOAD_SIZE)

	pkt_status_code status;

	uint8_t ackWindow = 1;
	int desc = 0;
	if(socket > in){
		desc = socket+1;
	}
	else{
		desc = in+1;
	}
	pkt_t *last = NULL;

	while((a != 0 || i != 31) && lastIsReceived !=1){
		FD_ZERO(&read_fds);
		FD_SET(in, &read_fds);//we can read from the input file/stdin or from the socket
		FD_SET(socket, &read_fds);
		FD_ZERO(&write_fds);
		FD_SET(socket, &write_fds);//we only write on the socket

		bufSize = 524; //reset the value at each loop;
		if(select(desc, &read_fds, &write_fds, NULL, 0)==-1){
			fprintf(stderr, "error: %s\n", strerror(errno));
		}

		if(FD_ISSET(in, &read_fds) && FD_ISSET(socket, &write_fds) && i>0 && sentPacket < ackWindow){
			a = read(in, (void *)temp, MAX_PAYLOAD_SIZE);
			if(a<0){
				fprintf(stderr, "error: %s\n", strerror(errno));
			}
			if(a == 0 && i==31 && encodeLast == 0){
				last = pkt_new();
				pkt_set_type(last, PTYPE_DATA);
				pkt_set_window(last,0);
				pkt_set_seqnum(last,seqnum%256);
				seqnumLast = seqnum%256+1;
				pkt_set_timestamp(last,(uint32_t)time(NULL));
				pkt_set_length(last,0);
				status = pkt_encode(last, buf, &bufSize);
				printf("encode last\n");
				window[0] = last;
				lastIsSend = 1;
				encodeLast = 1;

				if(status != 0){
					fprintf(stderr, "error at encode : %d\n", status);
				}

				a = write(socket, (void *)buf, bufSize);


			}
			else if(lastIsSend == 1 && window[0] !=NULL){
				if(window[0] != NULL && time(NULL)-pkt_get_timestamp(window[0]) > 6 && ackWindow >= sentPacket){
					pkt_set_timestamp(window[0],(uint32_t)time(NULL));
						status = pkt_encode(window[0], buf, &bufSize); //encode the pkt to be sent

						if(status != 0){
							fprintf(stderr, "error at encode : %d\n", status);
						}
						a = write(socket, (void *)buf, bufSize);
						if(a<0){
							fprintf(stderr, "error: %s\n", strerror(errno));
						}
				}
			}
			else if(a>0){

				pkt_t *send = pkt_new(); //creation of a structure to send (it needs to be encoded)
				pkt_set_payload(send, temp, a);
				if(pkt_get_length(send) > 0){
					pkt_set_type(send, PTYPE_DATA);
					pkt_set_window(send,0);
					pkt_set_seqnum(send,seqnum%256);

					for(int j = 0, boolFor = 0; j<31 && boolFor == 0; j++){
						if(window[j] == NULL){
							window[j] = send;
							i--;
							boolFor = 1;
						}
					}
					pkt_set_timestamp(send,(uint32_t)time(NULL));
					printf("send seqnum : %d, send length : %d\n",pkt_get_seqnum(send),pkt_get_length(send));
					status = pkt_encode(send, buf, &bufSize); //encode the pkt to be sent

					if(status != 0){
						fprintf(stderr, "error at encode : %d\n", status);
					}
					a = write(socket, (void *)buf, bufSize);
					if(a<0){
						fprintf(stderr, "error: %s\n", strerror(errno));
					}
					sentPacket++;
					ackWindow--;
					seqnum++;
				}
			}
		 }
		 if(FD_ISSET(socket, &read_fds)){

			if(lastIsSend == 1){
				a = read(socket, (void *)buf,bufSize);
				if(a < 0){
					fprintf(stderr, "error: %s\n", strerror(errno));
				}

				pkt_t *received = pkt_new();
				status = pkt_decode(buf, a, received);
				if(status != 0){
					fprintf(stderr,"Error : decode ack %d\n",status);
				}
				ackWindow = pkt_get_window(received);
				printf("ack recu : window %d seqnum : %d\n",ackWindow,pkt_get_seqnum(received));
				if(pkt_get_seqnum(received) == seqnumLast){
					printf("lastReceived\n");
					lastIsReceived = 1;
				}
				delWindow(window, pkt_get_seqnum(received)-1, &i, &sentPacket);
				pkt_del(received);

			}
			else{
				a = read(socket, (void *)buf,bufSize);
				if(a < 0){
					fprintf(stderr, "error: %s\n", strerror(errno));
				}

				pkt_t *received = pkt_new();
				status = pkt_decode(buf, a, received);
				if(status != 0){
					fprintf(stderr,"Error : decode ack %d\n",status);
				}
				ackWindow = pkt_get_window(received);
				printf("ack recu : window %d seqnum : %d\n",ackWindow,pkt_get_seqnum(received));

				delWindow(window, pkt_get_seqnum(received)-1, &i, &sentPacket);
				pkt_del(received);
			}

		}
		if (FD_ISSET(socket, &write_fds) && lastIsReceived != 1) {

			for(int j = 0; j<31 && lastIsReceived !=-1; j++){
				if(window[j] != NULL && time(NULL)-pkt_get_timestamp(window[j]) > 1 /*&& ackWindow >= sentPacket*/){

					pkt_set_timestamp(window[j],(uint32_t)time(NULL));
					status = pkt_encode(window[j], buf, &bufSize); //encode the pkt to be sent

					if(status != 0){
						fprintf(stderr, "error at encode : %d\n", status);
					}
					printf("seq timer %d\n", pkt_get_seqnum(window[j]));
					a = write(socket, (void *)buf, bufSize);
					if(a<0){
						fprintf(stderr, "error: %s\n", strerror(errno));
					}
			}
		}
	}


	}
	pkt_del(last);
	


}


void read_loop(const int socket, const int out){

	fd_set read_fds;
	fd_set write_fds;

	char buf[524]; //buf where the pkt is decoded
	char buf2[12]; //buf where the pkt is encoded

	pkt_t *window[MAX_WINDOW_SIZE];//buffer of struct waiting to be written on stdout
	for(int i = 0; i<MAX_WINDOW_SIZE;i++){
		window[i] = NULL;
	}

	int i = 0;//i is the number of occupied place in the array window
	int a = -1;
	int expected = 0;
	int boolack = 0;

	size_t bufSize = 524;
	uint16_t lengthR = 1;
	pkt_status_code status;

	pkt_t *received = NULL;
	pkt_t *ack = NULL;

	int desc = 0;
	if(socket > out){
		desc = socket+1;
	}
	else{
		desc = out+1;
	}

	while(i != 0 || lengthR != 0){
		FD_ZERO(&read_fds);//we can only read from the socket
		FD_SET(socket, &read_fds);
		FD_ZERO(&write_fds);
		FD_SET(socket, &write_fds);//we only write on the socket
		FD_SET(out, &write_fds);

		if(select(desc, &read_fds, &write_fds, NULL, 0)==-1){
			fprintf(stderr, "error: %s\n", strerror(errno));
		}
		
		if(FD_ISSET(socket, &read_fds) && FD_ISSET(out,&write_fds) && i != 31){
			a = read(socket, (void *)buf,bufSize);
				printf("a = %d\n", a);
			if(a < 0){
				fprintf(stderr, "error: %s\n", strerror(errno));
			}
			received = pkt_new();
			status = pkt_decode(buf,a,received);
			if (status != 0) {
				fprintf(stderr, "Error decode : %d, address : %d\n",status, received == NULL);

				if(status == E_CRC){
						lengthR = 1;
						pkt_del(received);
						received = NULL;
				}
			}
			if(received != NULL && expected%256 == pkt_get_seqnum(received)){
				pkt_t *last = received;
				uint16_t lengthReceived = pkt_get_length(received);
				a = write(out,pkt_get_payload(received), pkt_get_length(received));
				if(a<0){
					fprintf(stderr,"Error : %s",strerror(errno));
				}
				int expectedSeq = pkt_get_seqnum(received) + 1;
				int boolWhile = 0;
				lengthR = pkt_get_length(last);
				while (boolWhile == 0) {
					boolWhile = 1;
					for(int j = 0; j<31 ; j++){
						if(window[j] != NULL && pkt_get_seqnum(window[j]) == expectedSeq%256){
							a = write(out, pkt_get_payload(window[j]), pkt_get_length(window[j]));
							printf("something happen here?\n");
							i--;
							expectedSeq++;
							pkt_del(last);
							last = window[j];
							window[j] = NULL;
							boolWhile = 0;
							lengthR = pkt_get_length(last);
							boolack = 1;
							expected++;
						}
						else if(window[j] != NULL && pkt_get_seqnum(window[j]) < expectedSeq%256){

							pkt_del(window[j]);
							window[j] = NULL;
							i--;
						}
					}
				}
				//expected = expectedSeq;
				if(a < lengthReceived){
					fprintf(stderr,"Error : %s\n",strerror(errno));
				}else{
					printf("do we pass here?\n");
					if(ack != NULL){
						pkt_del(ack);
					}
					ack = pkt_new();
					create_ack(ack,last);
					pkt_del(last);
					boolack = 1;
					expected++;
				}
			}
			else if(received != NULL && expected%256 < pkt_get_seqnum(received)){
				printf("seqnum greater : %d vs fucking expected : %d\n", pkt_get_seqnum(received), expected%256);
				for(int j = 0, boolFor = 0; j<31 && boolFor == 0; j++){
					if(window[j] == NULL){
						window[j] = received;
						i++;
						boolFor = 1;
					}
					else if(pkt_get_seqnum(window[j]) == pkt_get_seqnum(received)){
						pkt_del(received);
						boolFor = 1;
					}
				}
			}
		}

		if(FD_ISSET(socket, &write_fds) && boolack == 1){

			size_t siZe = 12;
			printf("ack seqnum : %d\n", pkt_get_seqnum(ack));
			pkt_set_window(ack,(uint8_t)31-i);
			pkt_encode((const pkt_t *)ack,buf2,&siZe);
			a = write(socket,buf2,12);
			boolack = 0;
			//pkt_del(ack);

		}

	}


}
