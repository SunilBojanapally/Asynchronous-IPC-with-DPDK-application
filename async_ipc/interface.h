/* Common members between Linux Applications and DPDK processes */
#include <event2/event.h>

struct event_base *evbase;

typedef struct sock_s {
	int sockfd;
	struct sockaddr_in servaddr;
} sock_t;

typedef enum {
	SHOW_MSG,
	FREE_MSG,
	IPC_MSG_TYPE_1,
	IPC_MSG_TYPE_2,
	/* Add more IPC message types 
         * and corresponding message buffers
         */ 
} msg_type_t;

struct show_msg {
	char name[32];
	int msg_type;
	int all;
};

struct free_msg {
	char name[32];
	int msg_type;	
};

struct msg_buf_type1 {
	char m1_name[32];
	int  m1_data;	
};

struct msg_buf_type2 {
	char m2_name[32];
	char m2_data[256];
};

typedef struct generic_msg_s {
	union {
		struct show_msg      smsg;
		struct free_msg      fmsg;
		struct msg_buf_type1 m_type1;
		struct msg_buf_type2 m_type2;
		/* Add more message structures per IPC messages type */
	} generic_msg_req;
} generic_msg_t;

typedef struct msg_s {
	msg_type_t         msg_type;
	generic_msg_t      msg;	
} msg_t;

int ipc_thread_init();
int process_ipc_msg_type1_request(void *);
int process_ipc_msg_type2_request(void *);

