#ifndef __TCP_SESSION_H__
#define __TCP_SESSION_H__

#define MAX_SEND_PKG 2048

struct session {
	char c_ip[32];
	int c_port;
	void* c_sock;
	int removed;
	int is_shake_hand;
	int socket_type;
	int is_server_session;
	// unsigned int s_key;
	unsigned int uid; // 用户uid
	struct session* next;
	// unsigned char send_buf[MAX_SEND_PKG]; // 90%发送的命令缓存
};

void init_session_manager(int socket_type, int protocal_type);
void exit_session_manager();


// 有客服端进来，保存这个sesssion;
struct session* save_session(void* c_sock, char* ip, int port);
void close_session(struct session* s);
void remove_session(struct session* s);

// 遍历我们session集合里面的所有session
void foreach_online_session(int(*callback)(struct session* s, void* p), void*p);

// 处理我们的数据
void clear_offline_session();
// end 

int 
get_socket_type();

int 
get_proto_type();

#endif

