#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#endif

#include "session.h"
#include "../netbus.h"

#define MAX_SESSION_NUM 6000
#define my_malloc malloc
#define my_free free

struct {
	struct session* online_session;

	struct session* cache_mem;
	struct session* free_list;


	int readed; // 当前已经从socket里面读取的数据;

	int has_removed;
	int socket_type;  // 0 表示TCP socket, 1表示是 websocket
	int protocal_type;// 0 表示二进制协议，size + 数据的模式
	               // 1,表示文本协议，以回车换行来分解收到的数据为一个包
}session_manager;

int 
get_socket_type() {
	return session_manager.socket_type;
}

int 
get_proto_type() {
	return session_manager.protocal_type;
}

extern void
socket_send_data(void* ud, unsigned char* buf, int nread);

static struct session* cache_alloc() {
	struct session* s = NULL;
	if (session_manager.free_list != NULL) {
		s = session_manager.free_list;
		session_manager.free_list = s->next;
	}
	else { // 调用系统的函数 malloc
		s = my_malloc(sizeof(struct session));
	}
	memset(s, 0, sizeof(struct session));

	return s;
}

static void cache_free(struct session* s) {
	// 判断一下，是从cache分配出去的，还是从系统my_malloc分配出去的？
	if (s >= session_manager.cache_mem && s < session_manager.cache_mem + MAX_SESSION_NUM) {
		s->next = session_manager.free_list;
		session_manager.free_list = s;
	}
	else { 
		my_free(s);
	}
	// 
}

void init_session_manager(int socket_type, int protocal_type) {
	memset(&session_manager, 0, sizeof(session_manager));

	session_manager.socket_type = socket_type;
	session_manager.protocal_type = protocal_type;
	
	// 将6000个session一次分配出来。
	session_manager.cache_mem = (struct session*)my_malloc(MAX_SESSION_NUM * sizeof(struct session));
	memset(session_manager.cache_mem, 0, MAX_SESSION_NUM * sizeof(struct session));
	// end 

	for (int i = 0; i < MAX_SESSION_NUM; i++) {
		session_manager.cache_mem[i].next = session_manager.free_list;
		session_manager.free_list = &session_manager.cache_mem[i];
	}
}

void exit_session_manager() {

}

struct session* 
save_session(void* c_sock, char* ip, int port) {
	struct session* s = cache_alloc();
	s->c_sock = c_sock;
	s->c_port = port;
	int len = (int)strlen(ip);
	if (len >= 32) {
		len = 31;
	}
	strncpy(s->c_ip, ip, len);
	s->c_ip[len] = 0;

	s->next = session_manager.online_session;
	session_manager.online_session = s;
	return s;
}

void foreach_online_session(int(*callback)(struct session* s, void* p), void*p) {
	if (callback == NULL) {
		return;
	}

	struct session* walk = session_manager.online_session;
	while (walk) {
		if (walk->removed == 1) {
			walk = walk->next;
			continue;
		}
		if (callback(walk, p)) {
			return;
		}
		walk = walk->next;
	}
}

void remove_session(struct session* s) {
	s->removed = 1;
	session_manager.has_removed = 1;
	clear_offline_session();
	printf("client %s:%d exit\n", s->c_ip, s->c_port);
}

extern void
on_connect_lost_entry(struct session* s);

void clear_offline_session() {
	if (session_manager.has_removed == 0) {
		return;
	}

	struct session** walk = &session_manager.online_session;
	while (*walk) {
		struct session* s = (*walk);
		if (s->removed) {
			*walk = s->next;
			s->next = NULL;
			
			on_connect_lost_entry(s);
			// 
			s->c_sock = NULL;
			// 释放session
			cache_free(s);
		}
		else {
			walk = &(*walk)->next;
		}
	}
	session_manager.has_removed = 0;
}

static void
ws_send_data(struct session* s, const unsigned char* pkg_data, unsigned int pkg_len) {
	int long_pkg = 0;
	unsigned char* send_buffer = NULL;


	int header = 1; // 0x81
	if (pkg_len <= 125) {
		header++;
	}
	else if (pkg_len <= 0xffff) {
		header += 3;
	}
	else {
		header += 9;
	}
	if (header + pkg_len > MAX_SEND_PKG) {
		long_pkg = 1;
		send_buffer = my_malloc(header + pkg_len);
	}
	else {
		// send_buffer = s->send_buf;
		long_pkg = 1;
		send_buffer = my_malloc(MAX_SEND_PKG);
	}

	unsigned int send_len;
	// 固定的头
	send_buffer[0] = 0x81;
	if (pkg_len <= 125) {
		send_buffer[1] = pkg_len; // 最高bit为0，
		send_len = 2;
	}
	else if (pkg_len <= 0xffff) {
		send_buffer[1] = 126;
		// send_buffer[2] = (pkg_len & 0x000000ff);
		// send_buffer[3] = ((pkg_len & 0x0000ff00) >> 8);
		// 注意，与我们通常的内存存储不一样，低位存到高地址，高位存到低地址
		send_buffer[2] = ((pkg_len & 0x0000ff00) >> 8);
		send_buffer[3] = (pkg_len & 0x000000ff);

		send_len = 4;
	}
	else {
		send_buffer[1] = 127;
		send_buffer[5] = (pkg_len & 0x000000ff);
		send_buffer[4] = ((pkg_len & 0x0000ff00) >> 8);
		send_buffer[3] = ((pkg_len & 0x00ff0000) >> 16);
		send_buffer[2] = ((pkg_len & 0xff000000) >> 24);

		send_buffer[6] = 0;
		send_buffer[7] = 0;
		send_buffer[8] = 0;
		send_buffer[9] = 0;
		send_len = 10;
	}
	memcpy(send_buffer + send_len, pkg_data, pkg_len);
	send_len += pkg_len;
	
	socket_send_data(s->c_sock, send_buffer, send_len);
	
	if (long_pkg) {
		my_free(send_buffer);
	}
}

static void
tcp_send_data(struct session* s, const unsigned char* body, int len) {
	int long_pkg = 0;
	unsigned char* pkg_ptr = NULL;

	if (len + 2 > MAX_SEND_PKG) {
		pkg_ptr = my_malloc(len + 2);
		long_pkg = 1;
	}
	else {
		// pkg_ptr = s->send_buf;
		long_pkg = 1;
		pkg_ptr = my_malloc(MAX_SEND_PKG);
	}

	if (session_manager.protocal_type == JSON_PROTOCAL) {
		memcpy(pkg_ptr, body, len);
		strncpy(pkg_ptr + len, "\r\n", 2);
		socket_send_data(s->c_sock, pkg_ptr, len + 2);
	}
	else if (session_manager.protocal_type == BIN_PROTOCAL) {
		memcpy(pkg_ptr + 2, body, len);
		pkg_ptr[0] = ((len + 2)) & 0x000000ff;
		pkg_ptr[1] = (((len + 2)) & 0x0000ff00) >> 8;
		
		socket_send_data(s->c_sock, pkg_ptr, len + 2);
	}

	if (long_pkg && pkg_ptr != NULL) {
		my_free(pkg_ptr);
	}
}

void
session_send(struct session* s, unsigned char* body, int len) {

	if (s->socket_type == TCP_SOCKET_IO) {
		tcp_send_data(s, body, len);
	}
	else if (s->socket_type == WEB_SOCKET_IO) {
		ws_send_data(s, body, len);
	}
}

void
session_send_json(struct session* s, json_t* json) {
	char* json_str = NULL;
	json_tree_to_string(json, &json_str);
	session_send(s, json_str, (int)strlen(json_str));
	json_free_str(json_str);
}
