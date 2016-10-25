#include "packet_interface.h"
#include <zlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdint.h>



struct __attribute__((__packed__)) pkt {
	ptypes_t type:3;
	uint8_t window:5;
	uint8_t seqnum;
	uint16_t length;
	uint32_t timestamp;
	char *payload;
	uint32_t crc;
};


pkt_t* pkt_new()
{
	pkt_t *new =(pkt_t *)calloc(sizeof(pkt_t),1);
	new->payload = NULL;
	return new;
}

void pkt_del(pkt_t *pkt)
{
	if(pkt->payload != NULL){
		free(pkt->payload);
	}
	free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	int copied = 0;
  if((data[0]>>5) == 1){
      pkt_set_type(pkt,PTYPE_DATA);
  }
  else if((data[0]>>5) == 2){
      pkt_set_type(pkt, PTYPE_ACK);
  }
  else{
	//fprintf(stderr,"type decode %d \n", data[0]);
      return E_TYPE;
  }
  uint8_t window;
  memcpy((void *)&window, data, sizeof(uint8_t));
  window =  (window<<3);
  window = window >>3;
  pkt_set_window(pkt,window);
  pkt_set_seqnum(pkt, data[1]);
  copied+=2;
	uint16_t length;
	memcpy(&length,data+2,sizeof(uint16_t));
	copied+=sizeof(uint16_t);
	pkt_set_length(pkt,ntohs(length));

	uint32_t timestamp;
	memcpy(&timestamp, data+4, sizeof(uint32_t));
	copied+=sizeof(uint32_t);
	pkt_set_timestamp(pkt,(timestamp));
	if(pkt_get_length(pkt) !=0){
		char *bufTemp = (char *)calloc(1, pkt->length);
		memcpy(bufTemp, data+8, pkt->length);
		pkt_set_payload(pkt, bufTemp,pkt->length);
		free(bufTemp);
		copied+=pkt->length;
}

	uint32_t crcComp = (crc32(0,(const Bytef *) data, copied));

	uint32_t crc;
	memcpy(&crc, data+8+(pkt->length), sizeof(uint32_t));
	copied+=sizeof(uint32_t);
	pkt_set_crc(pkt,ntohl(crc));

	if(crcComp != pkt->crc){
	
		return E_CRC;
	}
	else if((int)len != copied){
		return E_UNCONSISTENT;
	}
	else if(pkt->type != PTYPE_DATA && pkt->type != PTYPE_ACK){
		return E_TYPE;
	}
	else{
		// fprintf(stderr,"type decode %d \n", pkt_get_type(pkt));
        //fflush(stderr);
		return PKT_OK;
	}
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{

	if(*len< ((size_t)12+pkt->length)){
		return E_NOMEM;
	}
	else{

      	*len = 0;
        memset(buf,0,12+pkt_get_length(pkt));
		buf[0] = (pkt_get_type(pkt)<<5)+pkt_get_window(pkt);
		*len+=1;
        uint8_t seq = pkt_get_seqnum(pkt);
        memcpy((void *)buf+1,(void *) &seq, 1);
        *len+=1;
		uint16_t length = htons(pkt_get_length(pkt));

   		memcpy((void *)buf+2,(void *) &length, 2);
		*len+=2;
		uint32_t timestamp = (pkt_get_timestamp(pkt));
		memcpy((void *)buf+4,(void *) &timestamp, 4);
		*len+=4;
		if(pkt_get_payload(pkt) != NULL){
			memcpy((void *)buf+8,(void *) pkt->payload, pkt_get_length(pkt));
			*len+=pkt_get_length(pkt);
		}
		uint32_t crc =htonl((uint32_t)crc32(0,(const Bytef *)buf,8+pkt_get_length(pkt)));
		memcpy(buf+8+(pkt->length), &crc, sizeof(uint32_t));
        *len+=4;
		//fprintf(stderr,"ok\n");
		return PKT_OK;
	}
}

ptypes_t pkt_get_type  (const pkt_t *pkt)
{
	return pkt->type;
}

uint8_t  pkt_get_window(const pkt_t *pkt)
{
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t *pkt)
{
	return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t *pkt)
{
	return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t *pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc(const pkt_t *pkt)
{
	return pkt->crc;
}

const char* pkt_get_payload(const pkt_t *pkt)
{
	return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if(type != 1 && type != 2){
		return E_TYPE;
	}
	else{
		pkt->type = type;
		return PKT_OK;
	}
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if(window>MAX_WINDOW_SIZE){
		return E_WINDOW;
	}
	else{
		pkt->window = window;
		return PKT_OK;
	}
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	pkt->seqnum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if(length>MAX_PAYLOAD_SIZE){
		return E_LENGTH;
	}
	else{
		pkt->length = length;
		return PKT_OK;
	}
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	pkt->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
	if(crc <= 0){
		return E_CRC;
	}
	else{
		pkt->crc = crc;
		return PKT_OK;
	}
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length)
{
	if(length >MAX_PAYLOAD_SIZE){
		return E_LENGTH;
	}
	else{
		pkt->length = length;
		pkt->payload = (char *)calloc(1,length);
		memcpy(pkt->payload, data, length);
		return PKT_OK;
	}
}
