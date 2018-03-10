#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "arpa/inet.h"

#define MSS 1460
#define RECV_WINDOW_SIZE 2048
struct TCP {
	int32_t dest_addr;
	int32_t dest_port;
	int32_t source_port;
	int sock;
	sockaddr_in addr;
	int32_t base_seq;
	int32_t next_seq;
	int32_t recv_window_head;
	int32_t recv_window_tail;
	char* recv_window;
	
};
struct TCPDataGram {
	int16_t source_port;
	int16_t destination_port;
	int32_t sequence_number_field;
	int32_t acknowledgment_number_field;
	int16_t options;
	int16_t receive_window;
	uint16_t cksum;
	int16_t urgen_ptr;
	char data[0];
};

int init_TCP(TCP* t, const char* address, const int port) {
	t->addr.sin_family = AF_INET;
	t->addr.sin_addr.s_addr = inet_addr(address);
	t->addr.sin_port = htons(port);

	if ( (t->sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("sock");
		return -1;
	}

	return 1;
}

uint16_t getCheckSum(TCPDataGram* datagram, size_t size) {
	//printf("size : %lu\n", size);
	unsigned char* t = (unsigned char*)datagram;
	for(int i = 0; i < size; i++) {
		printf("check %d : 0x%x\n", i, *t);
		t++;
	}
	size_t len;
	if (size % 0x4 == 0) {
		len = size;
	} else {
		len = ((size / 0x4) + 1) * 0x10;
	}
	printf("Len : %lu\n", len);
	char* buf = (char*) malloc(len);
	memset(buf, 0, len);
	memcpy(buf, (char*) datagram, size);
	uint16_t* buf16 = (uint16_t*) buf;

	uint32_t sums = 0;
	for (size_t i = 0; i < len / 0x2; i++) {
		//printf("%lu : 0x%x\n", i, *buf16);
		sums += *buf16;
		buf16++;
		sums = (sums & 0xffff) + (sums >> 16);
	}
	free(buf);
	buf = NULL;
	buf16 = NULL;
	//printf("CheckSum : 0x%x\n", sums);
	return (int16_t)(sums & 0xffff);
}

int listen(TCP* tcp, int16_t port){
	tcp->addr.sin_port = htons(port);
	tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);

	printf("Bind %s:%u\n", inet_ntoa(tcp->addr.sin_addr), htons(tcp->addr.sin_port));
	if ( bind(tcp->sock, (struct sockaddr *)&tcp->addr, sizeof(tcp->addr)) < 0) {
		perror("bind");
		return -1;
	}
	printf("listen init");
	char* buf = (char*)malloc( MSS + 40);
	
	size_t n;
	
	
	int16_t source_port;
	int16_t dest_port;
	bool SYN;
	uint16_t cksum;
	TCPDataGram* data;
	printf(".....");
	printf("done\n");
    struct sockaddr_in client_addr;
	int len = sizeof(client_addr);
	do {
		n = recvfrom(tcp->sock, buf, MSS + 40, 0, (struct sockaddr*)&client_addr, (socklen_t*) &len);
		printf("Recv %lu data from %s %u\n", n, inet_ntoa( client_addr.sin_addr), htons( client_addr.sin_port));
		for(int i = 0; i < n; i++) {
			printf("Recv %d : 0x%x\n", i, *buf);
			buf++;
		}
		buf = buf - n;
		data = (TCPDataGram*) buf;
		source_port = data->source_port;
		dest_port = data->destination_port;
		SYN = (data->options & 0x0002) >> 1;
		cksum = getCheckSum(data, sizeof(TCPDataGram));
		printf("Source_potr:%d\n", source_port);
		printf("SYN : %d\n", SYN);
		printf("CKSUM : 0x%x\n", cksum);
	} while( !SYN || (cksum ^ 0xffff) != 0);
	printf("It is availble\n");

	TCPDataGram* datagram = (TCPDataGram*) malloc(sizeof(TCPDataGram));
	memset(datagram, 0, sizeof(TCPDataGram));
	datagram->source_port = 12345;
	datagram->destination_port = data->source_port;
	datagram->sequence_number_field = rand() % 0xffffffff;
	datagram->acknowledgment_number_field = data->sequence_number_field + 1;
	datagram->options |= 0x5002;
	datagram->receive_window = RECV_WINDOW_SIZE;
	datagram->cksum = ~(getCheckSum(datagram, sizeof(TCPDataGram)));
	tcp->recv_window = (char*) malloc(RECV_WINDOW_SIZE);
	tcp->recv_window_head = tcp->recv_window_tail = 0;
	printf("Send ACK pakect\n");
	sendto(tcp->sock, datagram, sizeof(TCPDataGram), 0, (struct sockaddr*) &client_addr, sizeof(client_addr));
	//printf("%s %u Connected\n", inet_ntoa(tcp->addr.sin_addr), ntohs(tcp->addr.sin_port));
	return 0;
}

int connect(TCP* tcp, const char* dest, const int port) {
    tcp->addr.sin_addr.s_addr = inet_addr(dest);
	tcp->addr.sin_port = htons(port);
	tcp->addr.sin_family = AF_INET;
	
	tcp->base_seq = tcp->next_seq = rand() % (1 < 32);
	TCPDataGram* datagram = (TCPDataGram*) malloc(sizeof(TCPDataGram));
	memset(datagram, 0, sizeof(TCPDataGram));
	//should choose a availble port
	datagram->source_port = tcp->source_port = 10000;
	datagram->destination_port = tcp->dest_port;
	datagram->sequence_number_field = tcp->next_seq;
	/* set SYN bit
	 * option:
	 * 	head length(unit: 4 Byte) 	(4 bit)
	 * 	unused 						(6 bit)
	 * 	URG 						(1 bit)
	 * 	ACK 						(1 bit)
	 * 	PSH 						(1 bit)
	 * 	RST 						(1 bit)
	 * 	SYN 						(1 bit)
	 * 	FIN 						(1 bit)
	 */
	datagram->options = 0x5002;

	//printf("CKSUM: 0x%x\n", getCheckSum(datagram, sizeof(TCPDataGram)));
	datagram->cksum = ~ (getCheckSum(datagram, sizeof(TCPDataGram)));
	//printf("datagram->CKSUM: 0x%x\n", datagram->cksum);
	//printf("CKSUM: 0x%x\n", getCheckSum(datagram, sizeof(TCPDataGram)));

	printf("Send to %s:%u\n", inet_ntoa(tcp->addr.sin_addr), htons(tcp->addr.sin_port));
	unsigned char* t = (unsigned char*)datagram;
	for(int i = 0; i < 20; i++) {
		printf("Send %d : 0x%x\n", i, *t);
		t++;
	}
	
	sendto(tcp->sock,
	 (unsigned char*)datagram,
	 sizeof(TCPDataGram),
	 0,
	 (struct sockaddr*) &tcp->addr,
	 sizeof(tcp->addr));

	char* buf = (char*)malloc(MSS + 40);

	int len = sizeof(tcp->addr);
	int n = 0;
	int SYN = 0;
	int ack = 0;
	uint16_t cksums = 0;
	TCPDataGram* data = NULL;

	do {
		n = recvfrom(tcp->sock, buf, MSS + 40, 0,
		 					(struct sockaddr*)&tcp->addr, (socklen_t*)&len);
	void* d = malloc(n);
	memcpy(d, buf, n);
	data = (TCPDataGram*) d;

	SYN = (data->options & 0x0002) >> 1;
	ack = data->acknowledgment_number_field;
	cksums = getCheckSum(data, n);
	
	printf("Recv Data\n");
	printf("SYN : %d\n", SYN);
	printf("CKSUMS : 0x%x\n", cksums);
	} while(SYN != 1 || (cksums ^ 0xffff) != 0); 
	
	// throw this packet
	/* SYN equal to one
	 * ACK equal to one
	 * CKSUM equal to 0xffff

	 */
	
	// set up the receive window
	tcp->recv_window = (char*) malloc(RECV_WINDOW_SIZE);
	tcp->recv_window_head = tcp->recv_window_tail = 0;

	memset(datagram, 0, sizeof(TCPDataGram));
	datagram->source_port = tcp->source_port;
	datagram->destination_port = tcp->dest_port;
	datagram->sequence_number_field = data->acknowledgment_number_field;
	datagram->acknowledgment_number_field = data->sequence_number_field + 1;
	datagram->receive_window = RECV_WINDOW_SIZE -
		 (tcp->recv_window_tail - tcp->recv_window_head + RECV_WINDOW_SIZE) 
		 	% RECV_WINDOW_SIZE;
	datagram->options |= 0x5000;

	sendto(tcp->sock, (void*)datagram, sizeof(TCPDataGram), 0, (struct sockaddr*)&tcp->addr, sizeof(tcp->addr));
	
	printf("Client Connected\n");

	// init recv_window;
	// save sequence_number
	// base + 1 = next + 1 = sequence_number;
	// send the third packet
	return 0;
}

int close(TCP* t) {
	return 0;
}

#ifdef RUN
int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Usage: %s port\n", argv[0]);
		exit(1);
	}

	printf("Welcome! This is a UDP server, I can only received message from client and reply with same message\n");

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int sock;

	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0) ) < 0) {
		perror("socket");
		exit(1);
	}

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("bind");
			exit(1);
	}

	char buff[512];
	struct sockaddr_in clientAddr;
	int n;
	int len = sizeof(clientAddr);

	while(1) {
			n = recvfrom(sock, buff, 511, 0, (struct sockaddr*)&clientAddr, &len);
			if (n > 0) {
					buff[n] = 0;
					printf("%s %u says: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buff);
					n = sendto(sock, buff, n, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
					if (n < 0) {
						perror("sendto");
						break;
					}
			}
			else {
					perror("recv");
					break;
			}
	}

	return 0;
}
#endif