#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "tinyTCP.h"

/*
	if (argc != 3) {
		printf("usage: %s ip port", argv[0]);
		exit(1);
	}

	printf("This is a UDP client\n");
	struct sockaddr_in addr;
	int sock;
	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("socket");
			exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(argv[2]));

	if (addr.sin_addr.s_addr == INADDR_NONE) {
			printf("Incorrect ip address!");
			close(sock);
			exit(1);
	}

	char buff[512];
	int len = sizeof(addr);
	while(1) {
			gets(buff);
			int n;
			n = sendto(sock, buff, strlen(buff), 0, (struct sockaddr *)&addr, sizeof(addr));
			if (n < 0) {
					perror("sendto");
					close(sock);
					break;
			}

			n = recvfrom(sock, buff, 512, 0, (struct sockaddr*)&addr, (socklen_t*)&len);
			if (n > 0) {
					buff[n] = 0;
					printf("received:");
					puts(buff);
			}

			else if (n == 0) {
					printf("server closed\n");
					close(sock);
					break;
			}
			else if ( n  == -1) {
					perror("recvfrom");
					close(sock);
					break;
			}
	}
*/

int main(int argc, char** argv) {
	TCP tcp;
	init_TCP(&tcp, "127.0.0.1", 80);
	connect(&tcp, "127.0.0.1", 12345);

	return 0;
}
