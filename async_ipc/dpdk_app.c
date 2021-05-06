#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_mbuf.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_hash.h>
#include <rte_malloc.h>
#include <rte_mempool.h>
#include <rte_errno.h>
#include <rte_jhash.h>

#include "interface.h"

#define MSG_BUF_TYPE1_SIZE sizeof(struct msg_buf_type1)
#define MSG_BUF_TYPE2_SIZE sizeof(struct msg_buf_type2)

static struct rte_mempool   *mem_msg_mtype1 = NULL;
static struct rte_mempool   *mem_msg_mtype2 = NULL;
static struct rte_hash      *msg_mtype1_hash_table = NULL;
static struct rte_hash      *msg_mtype2_hash_table = NULL;
static struct msg_buf_type1 **msg_mtype1_hash_map = NULL;
static struct msg_buf_type2 **msg_mtype2_hash_map = NULL;

/* Allocate mempool for IPC Msg structures */
static int lcore_mem_init() {
	char name[32] = {'\0'};
	char hash_name[32]    = {'\0'};
	uint32_t max_ipc_msgs = 1024;

 	/* Allocation for mmepool msg buf type1 */
	snprintf(name, sizeof(name), "mempool-ipc-msg-type1");	
	mem_msg_mtype1 = rte_mempool_lookup(name);
	if (mem_msg_mtype1 == NULL) {
		mem_msg_mtype1 = rte_mempool_create(name,
						max_ipc_msgs,
						MSG_BUF_TYPE1_SIZE,
						0, 0, NULL, NULL, NULL, NULL, rte_socket_id(),
						MEMPOOL_F_SP_PUT);
	}

	if (mem_msg_mtype1 == NULL) {
		RTE_LOG(ERR, USER1,  "Failure initializing mempool for %s %s",
				name, rte_strerror(rte_errno));
		return -1;
	}

	struct rte_hash_parameters hash_params = {
		.name = "ipc-msg-type1-hashtable",
		.entries = max_ipc_msgs,
		.key_len = sizeof(name),
		.hash_func = rte_jhash,
		.hash_func_init_val = 0,
		.socket_id = rte_socket_id(),
	};

	msg_mtype1_hash_table = rte_hash_find_existing(hash_params.name);
	if (msg_mtype1_hash_table == NULL) {
		msg_mtype1_hash_table = rte_hash_create(&hash_params);
		if (unlikely(msg_mtype1_hash_table == NULL)) {
			RTE_LOG(ERR, USER1, "Could not allocate"
				"hash table for IPC msg type1. Error %s\n", rte_strerror(rte_errno));
			return -1;
		}
	}

	snprintf(name, sizeof(name), "ipc-msg-type1-hashmap");
	msg_mtype1_hash_map = rte_zmalloc(name, max_ipc_msgs * MSG_BUF_TYPE1_SIZE, 0);
	if (unlikely(msg_mtype1_hash_map == NULL)) {
		RTE_LOG(ERR, USER1, "Could not allocate"
			"hashmap entries for IPC msg type1. Error %s\n", rte_strerror(rte_errno));
		return -1;
	}

	/* Allocation for mempool msg buf type2 */
	snprintf(name, sizeof(name), "mempool-ipc-msg-typer2");
	mem_msg_mtype2 = rte_mempool_lookup(name);
	if (mem_msg_mtype2 == NULL) {
		mem_msg_mtype2 = rte_mempool_create(name,
						max_ipc_msgs,
						MSG_BUF_TYPE2_SIZE,
						0, 0, NULL, NULL, NULL, NULL, rte_socket_id(),
						MEMPOOL_F_SP_PUT);
	}

	if (mem_msg_mtype2 == NULL) {
		RTE_LOG (ERR, USER1,  "Failure initializing mempool for %s %s",
                                name, rte_strerror(rte_errno));
		return -1;
	}

	hash_params.name = "ipc-msg-type2-hashtable";

	msg_mtype2_hash_table = rte_hash_find_existing(hash_params.name);
	if (msg_mtype2_hash_table == NULL) {
		msg_mtype2_hash_table = rte_hash_create(&hash_params);
		if (unlikely(msg_mtype2_hash_table == NULL)) {
			RTE_LOG (ERR, USER1, "Could not allocate"
				"hash table for IPC msg type2. Error %s\n", rte_strerror(rte_errno));
			return -1;
		}
	}

	snprintf(name, sizeof(name), "ipc-msg-type2-hashmap");
	msg_mtype2_hash_map = rte_zmalloc(name, max_ipc_msgs * MSG_BUF_TYPE2_SIZE, 0);
	if (unlikely(msg_mtype2_hash_map == NULL)) {
		RTE_LOG (ERR, USER1, "Could not allocate"
			"hashmap entries for IPC msg type2. Error %s\n", rte_strerror(rte_errno));
		return -1;
	}
	return 0;
}

int show_ipc_msg(void *show_req) {
	int32_t ret;
	char name[32] = {'\0'};
	struct show_msg *req = (struct show_msg *)show_req;

	strlcpy(name, req->name, sizeof(name));

	switch(req->msg_type) {
	case IPC_MSG_TYPE_1: {
		ret = rte_hash_lookup(msg_mtype1_hash_table, (void *)name);
		if (ret >= 0) {
			struct msg_buf_type1 *msg = msg_mtype1_hash_map[ret];
			RTE_LOG (INFO, USER1, "Entry found, name: %s data: %d\n", msg->m1_name, msg->m1_data);
			return 0;
		} else {
			RTE_LOG (INFO, USER1, "No entry found for %s\n", name);
			return -1;
		}
	} break;
	case IPC_MSG_TYPE_2: {
		ret = rte_hash_lookup(msg_mtype2_hash_table, (void *)name);
		if (ret >= 0) {
			struct msg_buf_type2 *msg = msg_mtype2_hash_map[ret];
			RTE_LOG (INFO, USER1, "Entry found, name: %s data: %s\n", msg->m2_name, msg->m2_data);
			return 0;
		} else {
			RTE_LOG (INFO, USER1, "No entry found for %s\n", name);
			return -1;
		}
	} break;
	default: {
		RTE_LOG (ERR, USER1, "Invalid IPC msg type\n");
		return -1;
	}
	}
}

int process_ipc_msg_cleanup(void *req) {
	int32_t del_key;
	struct free_msg *fmsg = (struct free_msg *)req; 
	switch(fmsg->msg_type) {
	case IPC_MSG_TYPE_1: {
		del_key = rte_hash_del_key(msg_mtype1_hash_table, (void *)fmsg->name);
		if (del_key < 0) {
			RTE_LOG(ERR, USER1, "Entry doesn't exists\n", __func__);
			return -1;
		}
		struct msg_buf_type1 *msg = msg_mtype1_hash_map[del_key];
		RTE_LOG(INFO, USER1, "Freeing mem & hash entry %s\n", msg->m1_name);
		rte_mempool_put(mem_msg_mtype1, msg);
		msg_mtype1_hash_map[del_key] = NULL;
	} break;
	case IPC_MSG_TYPE_2: {
		del_key = rte_hash_del_key(msg_mtype2_hash_table, (void *)fmsg->name);
		if (del_key < 0) {
			RTE_LOG(ERR, USER1, "Entry doesn't exists\n", __func__);
			return -1;
		}
		struct msg_buf_type2 *msg = msg_mtype2_hash_map[del_key];
		RTE_LOG(INFO, USER1, "Freeing mem & hash entry %s\n", msg->m2_name);
		rte_mempool_put(mem_msg_mtype2, msg);
		msg_mtype2_hash_map[del_key] = NULL;
	} break;
	default: break;
	}
	return 0;
	
}

int process_ipc_msg_type1_request(void *req_msg) {
	char name[32] = {'\0'};
	struct msg_buf_type1 *msg;
	struct msg_buf_type1 *req = (struct msg_buf_type1 *)req_msg;

	strlcpy(name, req->m1_name, 32);
	int32_t ret = rte_hash_lookup(msg_mtype1_hash_table, (void *)name);
	if (ret >= 0) {
		RTE_LOG (ERR, USER1, "%s: message already exists\n", __func__);
		return 0;
	} else {
		ret = rte_mempool_get(mem_msg_mtype1, (void **)&msg);
		if (ret < 0) {
			RTE_LOG (ERR, USER1, "%s: No space in mempool\n", __func__);
			return -1;
		}
		memset(msg, 0, sizeof(&msg));

		rte_memcpy(msg->m1_name, name, sizeof(name));
		msg->m1_data = req->m1_data;

		/* Add to hash map */
		int32_t key = rte_hash_add_key(msg_mtype1_hash_table, (void *)name);
		if (key < 0) {
			RTE_LOG (ERR, USER1, "%s: message add to map failed\n", __func__);
			rte_mempool_put(mem_msg_mtype1, msg);
			return -1;
		}
		msg_mtype1_hash_map[key] = msg; 
		RTE_LOG (INFO, USER1, "%s: %s, message add successful\n", __func__, msg->m1_name);
	}
	
	return 0;
}

int process_ipc_msg_type2_request(void *req_msg) {
	char name[32] = {'\0'};
	struct msg_buf_type2 *msg;
	struct msg_buf_type2 *req = (struct msg_buf_type2 *)req_msg;

	strlcpy(name, req->m2_name, sizeof(name));
	int32_t ret = rte_hash_lookup(msg_mtype2_hash_table, (void *)name);
	if (ret >= 0) {
		RTE_LOG (ERR, USER1, "%s: message already exists\n", __func__);
		return 0;
	} else {
		ret = rte_mempool_get(mem_msg_mtype2, (void **)&msg);
		if (ret < 0) {
			RTE_LOG (ERR, USER1, "%s: No space in mempool\n", __func__);
			return -1;
		}
		memset(msg, 0, sizeof(&msg));

		rte_memcpy(msg->m2_name, name, sizeof(name));
		rte_memcpy(msg->m2_data, req->m2_data, strlen(req->m2_data));

		/* Add to hash map */
		int32_t key = rte_hash_add_key(msg_mtype2_hash_table, (void *)name);
		if (key < 0) {
			RTE_LOG (ERR, USER1, "%s: message add to map failed\n", __func__);
			rte_mempool_put(mem_msg_mtype2, msg);
			return -1;
		}
		msg_mtype2_hash_map[key] = msg;
		RTE_LOG (INFO, USER1, "%s: %s, message add successful\n", __func__, msg->m2_name);
	}

	return 0;
}

static int lcore_main(__rte_unused void *arg)
{
	int ret;
	unsigned lcore_id;
	lcore_id = rte_lcore_id();

	ret = ipc_thread_init();
	if (ret != 0) {
		RTE_LOG (ERR, USER1, "ipc_thread_init failed\n");
		return -1;
	}

	ret = lcore_mem_init();
	if (ret != 0) {
		RTE_LOG (ERR, USER1, "lcore_mem_init failed\n");
		return -1;
	}

	/* Let the DPDK app run forever */
	while(1) { }
	return 0;
}

int
main(int argc, char **argv)
{
	int ret;
	unsigned lcore_id;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");


	/* call lcore_hello() on every worker lcore */
	RTE_LCORE_FOREACH_WORKER(lcore_id) {
		rte_eal_remote_launch(lcore_main, NULL, lcore_id);
	}

	rte_eal_mp_wait_lcore();

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
