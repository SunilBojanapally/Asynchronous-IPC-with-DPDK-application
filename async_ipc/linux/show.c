#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "../interface.h"

#define MAX_NUM_THREADS  2 

int socket_connect(sock_t *sk) {
	if ((sk->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("%s: socket open failed\n", __func__);
		return -1;
	}

	memset(&sk->servaddr, 0, sizeof(sk->servaddr));

	sk->servaddr.sin_family = AF_INET;
	sk->servaddr.sin_port = htons(5001);
	sk->servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if(connect(sk->sockfd, (struct sockaddr *)&(sk->servaddr), sizeof(sk->servaddr)) < 0)
	{
		close(sk->sockfd);
		sk->sockfd = -1;
		printf("%s: Error : Connect Failed\n", __func__);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Incorrect command arguments, <./prog name msg_type>\n");
		return;
	}

	sock_t sk;
	int ret = socket_connect(&sk);
	if (ret != 0) {
		printf("Failed socket connect %d\n", errno);
		return;
	}

	msg_t msg;
	msg.msg_type = SHOW_MSG;

	strcpy(msg.msg.generic_msg_req.smsg.name, (char *)argv[1]);
	msg.msg.generic_msg_req.smsg.msg_type = atoi(argv[2]);

	if (sendto(sk.sockfd, &msg, sizeof(msg_t),
		0, (struct sockaddr*)NULL, sizeof(sk.servaddr)) == -1) {
		printf("failed to sendto error %d", errno);
	}
	return 0;
}
