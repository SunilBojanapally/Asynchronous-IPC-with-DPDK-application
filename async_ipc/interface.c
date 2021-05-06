#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <event.h>

#include <rte_log.h>
#include <rte_malloc.h>

#include "interface.h"

struct event *recv_ev;
static int sock_fd = -1;
struct sockaddr_in srvaddr, cliaddr;

void send_response(int ret, msg_type_t type) {
	if (type == IPC_MSG_TYPE_1 | IPC_MSG_TYPE_2 | FREE_MSG) {
		if (ret == 0) {
			sendto(sock_fd, (const char *)"success", strlen("success"),
				MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
				sizeof(cliaddr));
		} else {
			sendto(sock_fd, (const char *)"failure", strlen("failure"),
				MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
				sizeof(cliaddr));
		}
	}
	return;
}

static void ipc_handler_get(int x, short int y, void *arg)
{
	struct event **ev = arg;
	socklen_t recv_len = sizeof(cliaddr);

	void *rcv_info = rte_zmalloc(NULL, sizeof(msg_t), 0);
	int ret = recvfrom(sock_fd, rcv_info, sizeof(msg_t),
			0, (struct sockaddr *)&cliaddr, &recv_len);
	if (ret <= 0) {
		RTE_LOG(ERR, USER1, "%s: Unable to read %d errno %d\n", __func__, ret, errno);
		rte_free(rcv_info);
		return;
	}

	msg_t *req_info = (msg_t *)rcv_info;

	switch(req_info->msg_type) {
	case SHOW_MSG:
		ret = show_ipc_msg((void *)&req_info->msg.generic_msg_req);
	break;
	case FREE_MSG:
		ret = process_ipc_msg_cleanup((void *)&req_info->msg.generic_msg_req);
		send_response(ret, FREE_MSG);
	break;
	case IPC_MSG_TYPE_1:
		ret = process_ipc_msg_type1_request((void *)&req_info->msg.generic_msg_req);
		if (ret != 0) {
			RTE_LOG(ERR, USER1, "%s: process IPC message type 1 request failed\n", __func__);
		}
		send_response(ret, IPC_MSG_TYPE_1);
	break;
	case IPC_MSG_TYPE_2:
		ret = process_ipc_msg_type2_request((void *)&req_info->msg.generic_msg_req);
		if (ret != 0) {
			RTE_LOG(ERR, USER1, "%s: ocess IPC message type 2 request failed\n", __func__);
		}
		send_response(ret, IPC_MSG_TYPE_2);
	break;
	default: break;
	}

	rte_free(rcv_info);

	if (event_add(*ev, NULL) < 0) {
		RTE_LOG(ERR, USER1, "%s: failed to reload ctrl recv event\n", __func__);
	}
	return;
}

int ipc_create_sock_fd(void)
{
	int ret = -1;

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (unlikely(sock_fd < 0)) {
		RTE_LOG(ERR, USER1, "%s: Error "
			"creating UNIX socket errno: %d\n", __func__, errno);
		goto err;
	}

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	srvaddr.sin_port = htons(5001);

	ret = bind(sock_fd, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
	if (ret < 0) {
		RTE_LOG(ERR, USER1, "%s: Fatal error "
			"bind UNIX socket errno: %s\n", __func__, strerror(errno));
		close(sock_fd);
		goto err;
	}

	evbase = event_base_new();
	if (evbase == NULL) {
		RTE_LOG(ERR, USER1, "%s: failed to create event base errno: %d\n", __func__, errno);
		goto err;
	}

	recv_ev = event_new(evbase, sock_fd, EV_READ, ipc_handler_get, &recv_ev);
	if (recv_ev == NULL) {
		RTE_LOG(ERR, USER1, "%s: failed to create event errno: %d\n", __func__, errno);
		goto err;
	}

	if(event_add(recv_ev, NULL) < 0) {
		RTE_LOG(ERR, USER1, "%s: failed to add event errno: %d\n", __func__, errno);
		goto err;
	}

	ret = 0;
err:
	return ret;
}

static void *ipc_task_handle() {
	int ret = ipc_create_sock_fd();
	if (ret == -1) {
		return NULL;
	}
	event_base_dispatch(evbase);
}

int ipc_thread_init()
{
	pthread_t tid;
	unsigned lcore_id;
	lcore_id = rte_lcore_id();

	if (rte_ctrl_thread_create(&tid, "ipc-task",
		NULL, ipc_task_handle, NULL) != 0) {
 		RTE_LOG(ERR, USER1, "%s: Error "
			"creating FP ctrl task %d\n", __func__, errno);
		close(sock_fd);
		return -1;
	}

	return 0;
}
