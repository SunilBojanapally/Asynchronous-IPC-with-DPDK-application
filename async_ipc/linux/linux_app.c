/* Application code which spwans 2 threads 
 * as an example simulation. Threads connect 
 * to server socket and send messages to DPDK 
 * process.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "../interface.h"

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
		printf("%s: Socket connect Failed\n", __func__);
		return -1;
	}

	return 0;
}

void *thread1(void *tid) {
	printf("Thread: %s\n", (char *)tid);

	sock_t sk;
	uint8_t i = 1, n, len;
	char name[32] = {'\0'};
	char buf[32] = {'\0'};

	int ret = socket_connect(&sk);
	if (ret != 0) {
		printf("%s: Failed socket connect %d\n", __func__, errno);
		return;
	}

	msg_t msg;
	msg.msg_type = IPC_MSG_TYPE_1;

	while (i <= 10) {
		snprintf(name, sizeof(name), "msg-th1-%d", i);
		memcpy(msg.msg.generic_msg_req.m_type1.m1_name, name, sizeof(name));
		msg.msg.generic_msg_req.m_type1.m1_data = i;
		if (sendto(sk.sockfd, &msg, sizeof(msg_t),
			0, (struct sockaddr*)NULL, sizeof(sk.servaddr)) == -1) {
			printf("%s: failed to sendto error %d", __func__, errno);
			return;
		}
		n = recvfrom(sk.sockfd, (char *)buf, sizeof(buf), 
			MSG_WAITALL, (struct sockaddr *) &sk.servaddr,
			(socklen_t *)sizeof(sk.servaddr));
		buf[n] = '\0';
		printf("%s: DPDK app response: %s\n", __func__, buf);
		i++;
		sleep(1);
	}

	sleep(60);

	i = 1;
	while(i <= 10) {
		msg.msg_type = FREE_MSG;
		msg.msg.generic_msg_req.fmsg.msg_type = IPC_MSG_TYPE_1;
		snprintf(name, sizeof(name), "msg-th1-%d", i);
		memcpy(msg.msg.generic_msg_req.fmsg.name, name, sizeof(name));
		if (sendto(sk.sockfd, &msg, sizeof(msg_t),
			0, (struct sockaddr*)NULL, sizeof(sk.servaddr)) == -1) {
			printf("%s: failed to sendto error %d", __func__, errno);
			return;
		}
		n = recvfrom(sk.sockfd, (char *)buf, sizeof(buf),
			MSG_WAITALL, (struct sockaddr *) &sk.servaddr,
			(socklen_t *)sizeof(sk.servaddr));
		buf[n] = '\0';
		printf("%s: Entry delete: %s\n", __func__, buf);
		i++;
	}

	close(sk.sockfd);
	pthread_exit(NULL);
	return;	
}

void *thread2(void *tid) {
	printf("Thread: %s\n", (char *)tid);

	sock_t sk;
	uint8_t i = 1, n, len;
	char name[32] = {'\0'};
	char data[256] = {'\0'};
	char buf[32] = {'\0'};

	int ret = socket_connect(&sk);
	if (ret != 0) {
		printf("%s: Failed socket connect %d\n", __func__, errno);
		return;
	}

	msg_t msg;
	msg.msg_type = IPC_MSG_TYPE_2;

	while (i <= 10) {
		snprintf(name, sizeof(name), "msg-th2-%d", i);
		memcpy(msg.msg.generic_msg_req.m_type2.m2_name, name, sizeof(name));
		snprintf(data, sizeof(data), "val-%d", i);
		memcpy(msg.msg.generic_msg_req.m_type2.m2_data, data, sizeof(data));
		if (sendto(sk.sockfd, &msg, sizeof(msg_t),
			0, (struct sockaddr*)NULL, sizeof(sk.servaddr)) == -1) {
			printf("%s: failed to sendto error %d", __func__, errno);
			return;
		}
		n = recvfrom(sk.sockfd, (char *)buf, sizeof(buf),
			MSG_WAITALL, (struct sockaddr *) &sk.servaddr,
			(socklen_t *)sizeof(sk.servaddr));
		buf[n] = '\0';
		printf("%s: DPDK app response: %s\n", __func__, buf);
		i++;
		sleep(1);
	}

	sleep(60);

	i = 1;
	while(i <= 10) {
		msg.msg_type = FREE_MSG;
		msg.msg.generic_msg_req.fmsg.msg_type = IPC_MSG_TYPE_2;
		snprintf(name, sizeof(name), "msg-th2-%d", i);
		memcpy(msg.msg.generic_msg_req.fmsg.name, name, sizeof(name));
		if (sendto(sk.sockfd, &msg, sizeof(msg_t),
			0, (struct sockaddr*)NULL, sizeof(sk.servaddr)) == -1) {
			printf("%s: failed to sendto error %d", __func__, errno);
			return;
		}
		n = recvfrom(sk.sockfd, (char *)buf, sizeof(buf),
			MSG_WAITALL, (struct sockaddr *) &sk.servaddr,
			(socklen_t *)sizeof(sk.servaddr));
		buf[n] = '\0';
		printf("%s: Entry delete: %s\n", __func__, buf);
		i++;
	}
	close(sk.sockfd);
	pthread_exit(NULL);
	return;
}

int main() {
	pthread_t tid1, tid2;
	pthread_create(&tid1, NULL, thread1, (void*)"thread 1");
	pthread_create(&tid2, NULL, thread2, (void*)"thread 2");

	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	exit(0);
}
