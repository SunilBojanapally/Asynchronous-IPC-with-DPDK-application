# Asynchronous-IPC-with-DPDK-application

Asynchronous inter process communication between Linux applications and DPDK. 

Description:
--------------------
Async DPDK application initializes EAL layer, followed by IPC thread and mempool initializations. IPC thread is created by rte API `rte_ctrl_thread_create()` which listens on UDP socket whose callback function is registered and process it on a event notification from the Linux applications. Linux applications connect to server IPC thread on loopback interface with port `5001`. After connect is successful, applications send message buffers with unique message types and corresponding data. Based on the message type async dpdk application will process messages, updates runtime structures which were already created as part of mempool initializations. 

Linux applications and async DPDK application utilize interface as wrappers to communicate between them. Interface consists of data structures and IPC socket initializations. Refer to `interface.h` file example message types and corresponding data structures which are passed to DPDK application. For instance, message type `IPC_MSG_TYPE_1` will have a corresponding structure which holds data such as `struct msg_buf_type1`. Note, do not be misled by the almost too descriptive name assigned to the message data element m1_data. This field is not restricted to holding only a integer, but any data, in any form. The field itself completely arbitrary, since this structure gets redefined by the application programmer as required based on need. 

```
struct msg_buf_type1 {
	char m1_name[32];    /* unique identifier of a message */
	int  m1_data;        /* The message data itself */
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
	msg_type_t         msg_type;     /* type of message */
 	generic_msg_t      msg;          /* generic message data */
} msg_t;
```

Software versions:
--------------------
```
DPDK version: 21.05.0-rc1
Linux Host:
[root@centos linux]# uname -a
Linux centos 3.10.0-957.el7.x86_64 #1 SMP Wed Aug 21 22:52:18 IST 2019 x86_64 x86_64 x86_64 GNU/Linux
GCC version:
[root@centos linux]# gcc --version
gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
```

DPDK install steps:
--------------------
```
cd dpdk
meson build
ninja -C build
sudo ninja -C build install
ldconfig
```

Check if libdpdk can be found by pkg-config:

```[root@centos linux]# pkg-config --modversion libdpdk```

The above command should return the DPDK version installed. If not found, export the path to the installed DPDK libraries:

Note: On some linux distributions, such as Fedora or Redhat, paths in `/usr/local` are not in the default paths for the loader. Therefore, on these distributions, `/usr/local/lib` and `/usr/local/lib64` should be added to a file in `/etc/ld.so.conf.d/` before running ldconfig as shown below. [Source](https://doc.dpdk.org/guides/linux_gsg/build_dpdk.html).

```
[root@centos linux]# cat /etc/ld.so.conf.d/kernel-3.10.0-957.el7.x86_64.conf
\# Placeholder file, no vDSO hwcap entries used in this kernel.
/usr/local/lib64/
```

After default path of loader is updated below command should results non zero number.

```
[root@centos linux]# ldconfig -p | grep librte | wc -l
314
```

Async IPC build steps:
-----------------------
As shown in below steps, clone repository under exampled folder in DPDK. Export DPDK pkgconfig file path for application to include as part of there build. As mentioned above the path can vary based on linux distributions. Make will compile async IPC which will create build directory.

```
[root@centos linux]# cd dpdk/examples
[root@centos linux]# git clone https://github.com/SunilBojanapally/Asynchronous-IPC-with-DPDK-application.git
[root@centos linux]# cp -r Asynchronous-IPC-with-DPDK-application/async_ipc .
[root@centos linux]# cd async_ipc
[root@centos linux]# export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
[root@centos linux]# make
[root@centos linux]# ls build
async_ipc  async_ipc-shared
[root@centos linux]# file build/async_ipc-shared
build/async_ipc-shared: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 2.6.32, BuildID[sha1]=854fa074e27e44e042090bce12d2b3b1aed08df3, not stripped
```

Run:
--------------------
1. Execute Async DPDK application as below,

```
[root@centos linux]# ./build/async_ipc -c 0x3 -n 4
```

2. On another tab, execute linux_app process, which is a test Linux application code. As an example Linux application will create 2 pthreads, connect to DPDK application, communicate data.<br/>   
```
[root@centos linux]# cd linux_app
[root@centos linux]# gcc linux_app.c -lpthread -o linux_app
[root@centos linux]# ./linux_app
```
