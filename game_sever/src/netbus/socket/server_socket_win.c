#include<stdio.h>
#include<string.h>
#include <stdlib.h>


#include <WinSock2.h>
#include <mswsock.h>

#include <windows.h>

#define my_malloc malloc
#define my_free free
#define my_realloc realloc

#define MAX_PKG_SIZE ((1<<16) - 1)
#define MAX_RECV_SIZE 2047
// #define MAX_RECV_SIZE 8

#pragma comment(lib, "ws2_32.lib")
// #pragma comment(lib, "WSOCK32.LIB")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")

#include "session.h"
#include "../../3rd/http_parser/http_parser.h"
#include "../../3rd/crypt/sha1.h"
#include "../../3rd/crypt/base64_encoder.h"
#include "../../utils/timer_list.h"

#include "../netbus.h"
#include "../../utils/log.h"

extern struct timer_list* GATEWAY_TIMER_LIST;


char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n" \
"Upgrade:websocket\r\n" \
"Connection: Upgrade\r\n" \
"Sec-WebSocket-Accept: %s\r\n" \
"WebSocket-Location: ws://%s:%d/chat\r\n" \
"WebSocket-Protocol:chat\r\n\r\n";

extern void
on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);

extern void
on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len);


extern void
init_server_gateway();

extern void
exit_server_gateway();

static LPFN_ACCEPTEX lpfnAcceptEx;
static LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;
enum {
	IOCP_ACCPET = 0,
	IOCP_RECV,
	IOCP_WRITE,
};

struct io_package {
	WSAOVERLAPPED overlapped;
	WSABUF wsabuffer;

	int opt; // 标记一下我们当前的请求的类型;
	SOCKET accpet_sock;
	int recved; // 收到的字节数;
	unsigned char* long_pkg;
	int max_pkg_len;
	unsigned char pkg[MAX_RECV_SIZE + 1];
};

void
socket_send_data(void* ud, unsigned char* buf, int nread) {
	send((SOCKET) ud, buf, nread, 0);
}

void
close_session(struct session*s) {
	if (s->c_sock) {
		closesocket(s->c_sock);
		s->c_sock = NULL;
	}
	remove_session(s);
}

static void
post_accept(SOCKET l_sock, HANDLE iocp, struct io_package* pkg) {


	pkg->wsabuffer.buf = pkg->pkg;
	pkg->wsabuffer.len = MAX_RECV_SIZE;
	pkg->opt = IOCP_ACCPET;

	DWORD dwBytes = 0;
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	int addr_size = (sizeof(struct sockaddr_in) + 16);
	pkg->accpet_sock = client;

	// AcceptEx(l_sock, client, pkg->wsabuffer.buf, 0/*pkg->wsabuffer.len - addr_size* 2*/,
	lpfnAcceptEx(l_sock, client, pkg->wsabuffer.buf, 0/*pkg->wsabuffer.len - addr_size* 2*/,
		addr_size, addr_size, &dwBytes, &pkg->overlapped);
}

static void
post_recv(SOCKET client_fd, HANDLE iocp) {
	// 异步发送请求;
	// 什么是异步? recv 8K数据，架设这个时候，没有数据，
	// 普通的同步(阻塞)线程挂起，等待数据的到来;
	// 异步就是如果没有数据发生，也会返回继续执行;
	struct io_package* io_data = my_malloc(sizeof(struct io_package));
	// 清0的主要目的是为了能让overlapped清0;
	memset(io_data, 0, sizeof(struct io_package));

	io_data->opt = IOCP_RECV;
	io_data->wsabuffer.buf = io_data->pkg;
	io_data->wsabuffer.len = MAX_RECV_SIZE;
	io_data->max_pkg_len = MAX_RECV_SIZE;

	// 发送了recv的请求;
	// 
	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	int ret = WSARecv(client_fd, &(io_data->wsabuffer),
		1, &dwRecv, &dwFlags,
		&(io_data->overlapped), NULL);
}

// 解析到头的回掉函数
static char header_key[64];
static char client_ws_key[128];
static int has_client_key = 0;
static int
on_header_field(http_parser* p, const char *at, size_t length) {
	length = (length < 63) ? length : 63;
	strncpy(header_key, at, length);
	header_key[length] = 0;
	// printf("%s:", header_key);
	return 0;
}

static int
on_header_value(http_parser* p, const char *at,
size_t length) {
	if (strcmp(header_key, "Sec-WebSocket-Key") != 0) {
		return 0;
	}
	length = (length < 127) ? length : 127;

	strncpy(client_ws_key, at, length);
	client_ws_key[length] = 0;
	// printf("%s\n", client_ws_key);
	has_client_key = 1;

	return 0;
}


static int
process_ws_shake_hand(struct session* ses, struct io_package* io_data, 
                     char* ip, int port) {
	http_parser p;
	http_parser_init(&p, HTTP_REQUEST);

	http_parser_settings s;
	http_parser_settings_init(&s);
	s.on_header_field = on_header_field;
	s.on_header_value = on_header_value;

	has_client_key = 0;
	http_parser_execute(&p, &s, io_data->pkg, io_data->recved);

	// 如果没有拿到key_migic,表示数据没有收完，我们去收握手的数据;
	if (has_client_key == 0) {
		ses->is_shake_hand = 0;
		if (io_data->recved >= MAX_RECV_SIZE) { // 不正常的握手包;
			close_session(ses);
			my_free(io_data);
			return -1;
		}
		DWORD dwRecv = 0;
		DWORD dwFlags = 0;
		if (io_data->long_pkg != NULL) {
			my_free(io_data->long_pkg);
			io_data->long_pkg = NULL;

		}

		io_data->max_pkg_len = MAX_RECV_SIZE;
		io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
		io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

		int ret = WSARecv((SOCKET)ses->c_sock, &(io_data->wsabuffer),
			1, &dwRecv, &dwFlags,
			&(io_data->overlapped), NULL);

		return -1;
	}

	// 回一个http的数据给我们的client,建立websocket链接
	static char key_migic[256];
	const char* migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	sprintf(key_migic, "%s%s", client_ws_key, migic);

	int sha1_size = 0; // 存放加密后的数据长度
	int base64_len = 0;
	uint8_t sha1_content[SHA1_DIGEST_SIZE];
	crypt_sha1(key_migic, (int)strlen(key_migic), sha1_content, &sha1_size);
	char* b64_str = base64_encode(sha1_content, sha1_size, &base64_len);
	// end 
	strncpy(key_migic, b64_str, base64_len);
	key_migic[base64_len] = 0;
	// printf("key_migic = %s\n", key_migic);

	// 将这个http的报文回给我们的websocket连接请求的客户端，
	// 生成websocket连接。
	static char accept_buffer[256];
	sprintf(accept_buffer, wb_accept, key_migic, ip, port);
	send((SOCKET)ses->c_sock, accept_buffer, (int)strlen(accept_buffer), 0);
	ses->is_shake_hand = 1;
	base64_encode_free(b64_str);

	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	if (io_data->long_pkg != NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;

	}

	io_data->recved = 0;
	io_data->max_pkg_len = MAX_RECV_SIZE;
	io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
	io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

	int ret = WSARecv((SOCKET)ses->c_sock, &(io_data->wsabuffer),
		1, &dwRecv, &dwFlags,
		&(io_data->overlapped), NULL);
	return 0;
}

static int
recv_ws_header(unsigned char* pkg_data, int pkg_len, 
               int* pkg_size, int* out_header_size) {
	// 第一个字节是头，已经判断，跳过;
	// end 

	unsigned char* mask = NULL;
	unsigned char* raw_data = NULL;
	
	if (pkg_len < 2) { // websocket 包头没有收完
		return -1;
	}

	unsigned int len = pkg_data[1];
	
	// 最高的一个bit始终为1,我们要把最高的这个bit,变为0;
	len = (len & 0x0000007f);
	if (len <= 125) { // 4个mask字节，头才算收完整；
		if (pkg_len < (2 + 4)) { // 无法解析出 大小与mask的值
			return -1;
		}
		mask = pkg_data + 2; // 头字节，长度字节
	}
	else if (len == 126) { // 后面两个字节表示长度；
		if (pkg_len < (4 + 4)) { // 1 + 1 + 2个长度字节 + 4 MASK
			return -1;
		}
		len = ((pkg_data[2]) | (pkg_data[3] << 8));
		mask = pkg_data + 2 + 2;
	}
	// 1 + 1 + 8(长度信息) + 4MASK
	else if (len == 127){ // 这种情况不用考虑,考虑前4个字节的大小，后面不管;
		if (pkg_len < 14) {
			return -1;
		}
		unsigned int low = ((pkg_data[2]) | (pkg_data[3] << 8) | (pkg_data[4] << 16) | (pkg_data[5] << 24));
		unsigned int hight = ((pkg_data[6]) | (pkg_data[7] << 8) | (pkg_data[8] << 16) | (pkg_data[9] << 24));
		if (hight != 0) { // 表示后四个字节有数据int存放不了，太大了，我们不要了。
			return -1;
		}
		len = low;
		mask = pkg_data + 2 + 8;
	}
	// mask 固定4个字节，所以后面的数据部分
	raw_data = mask + 4;
	*out_header_size = (int)(raw_data - pkg_data);
	*pkg_size = len + (*out_header_size);
	// printf("data length = %d\n", len);
	return 0;
}

static void
parser_ws_pack(struct session* s,
                      unsigned char* body, int body_len,
					  unsigned char* mask, int protocal_type) {
	// 使用mask,将数据还原回来；
	for (int i = 0; i < body_len; i++) {
		body[i] = body[i] ^ mask[i % 4]; // mask只有4个字节的长度，所以，要循环使用，如果超出，取余就可以了。
	}
	// end

	// 发送上去，命令终于分好了。
	if (protocal_type == JSON_PROTOCAL) {
		on_json_protocal_recv_entry(s, body, body_len);
	}
	else if (protocal_type == BIN_PROTOCAL){
		on_bin_protocal_recv_entry(s, body, body_len);
	}
	// end
}

static void
on_ws_pack_recved(struct session* s, struct io_package* io_data, int protocal_type) {
	// Step1: 解析数据的头，获取我们游戏的协议包体的大小;
	while (io_data->recved > 0) {
		int pkg_size = 0;
		int header_size = 0;
		if (recv_ws_header(io_data->pkg, io_data->recved, &pkg_size, &header_size) != 0) { // 继续投递recv请求，知道能否接收一个数据头;
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;

			io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
			io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

			int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}

		// Step2:判断数据大小，是否不符合规定的格式
		if (pkg_size >= MAX_PKG_SIZE) { // ,异常的数据包，直接关闭掉socket;
			close_session(s);
			my_free(io_data); // 释放这个socket使用的完成端口的io_data;
			break;
		}

		// 是否收完了一个数据包;
		if (io_data->recved >= pkg_size) { // 表示我们已经收到至少超过了一个包的数据；
			unsigned char* pkg_data = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;
			
			// 0x81
			if (pkg_data[0] == 0x88) { // 对方要关闭socket
				// unsigned char close_data[2] = {0x88, 0};
				// session_send(s, close_data, 2);
				close_session(s);
				break;
			}

			parser_ws_pack(s, pkg_data + header_size, 
				                  pkg_size - header_size, pkg_data + header_size - 4, protocal_type);

			if (io_data->recved > pkg_size) { // 1.5 个包
				memmove(io_data->pkg, io_data->pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}

			if (io_data->recved == 0) { // 重新投递请求
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
				io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

				int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}
		}
		else { // 没有收完一个数据包，所以我们直接投递recv请求;
			unsigned char* recv_buffer = io_data->pkg;
			if (pkg_size > MAX_RECV_SIZE) {
				if (io_data->long_pkg == NULL) {
					io_data->long_pkg = my_malloc(pkg_size + 1);
					memcpy(io_data->long_pkg, io_data->pkg, io_data->recved);
				}
				recv_buffer = io_data->long_pkg;
			}

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			io_data->wsabuffer.buf = recv_buffer + io_data->recved;
			io_data->wsabuffer.len = pkg_size - io_data->recved;

			int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}
		// end 
	}
}

static int
read_pkg_tail(unsigned char* pkg_data, int recv, int* pkg_size) {
	if (recv < 2) { // 不可能存放\r\n
		return -1;
	}

	int i = 0;
	*pkg_size = 0;

	while (i < recv - 1) {
		if (pkg_data[i] == '\r' && pkg_data[i + 1] == '\n') {
			*pkg_size = (i + 2);
			return 0;
		}
		i++;
	}

	return -1;
}

static int
recv_header(unsigned char* pkg, int len, int* pkg_size) {
	if (len <= 1) { // 收到的数据不能够将我们的包的大小解析出来
		return -1;
	}

	*pkg_size = (pkg[0]) | (pkg[1] << 8);
	return 0;
}

static void 
on_bin_protocal_recved(struct session* s, struct io_package* io_data) {
	// Step1: 解析数据的头，获取我们游戏的协议包体的大小;
	while (io_data->recved > 0) {
		int pkg_size = 0;
		if (recv_header(io_data->pkg, io_data->recved, &pkg_size) != 0) { // 继续投递recv请求，知道能否接收一个数据头;
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;

			io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
			io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

			int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}

		// Step2:判断数据大小，是否不符合规定的格式
		if (pkg_size >= MAX_PKG_SIZE) { // ,异常的数据包，直接关闭掉socket;
			close_session(s);
			my_free(io_data); // 释放这个socket使用的完成端口的io_data;
			break;
		}

		// 是否收完了一个数据包;
		if (io_data->recved >= pkg_size) { // 表示我们已经收到至少超过了一个包的数据；
			unsigned char* pkg_data = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;

			// on_server_recv(s, pkg_data + 2, pkg_size - 2);
			on_bin_protocal_recv_entry(s, pkg_data + 2, pkg_size - 2);

			if (io_data->recved > pkg_size) { // 1.5 个包
				memmove(io_data->pkg, io_data->pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}

			if (io_data->recved == 0) { // 重新投递请求
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
				io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

				int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}
		}
		else { // 没有收完一个数据包，所以我们直接投递recv请求;
			unsigned char* recv_buffer = io_data->pkg;
			if (pkg_size > MAX_RECV_SIZE) {
				if (io_data->long_pkg == NULL) {
					io_data->long_pkg = my_malloc(pkg_size + 1);
					memcpy(io_data->long_pkg, io_data->pkg, io_data->recved);
				}
				recv_buffer = io_data->long_pkg;
			}

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			io_data->wsabuffer.buf = recv_buffer + io_data->recved;
			io_data->wsabuffer.len = pkg_size - io_data->recved;

			int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}
		// end 
	}
}

static void
on_json_protocal_recved(struct session* s, struct io_package* io_data) {
	while (io_data->recved > 0) {
		int pkg_size = 0;
		unsigned char* pkg_data = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;

		if (read_pkg_tail(pkg_data, io_data->recved, &pkg_size) != 0) { // 没有读到\r\n
			if (io_data->recved >= (((1 << 16) - 1))) { // 超过了数据包,close session
				close_session(s);
				my_free(io_data);
				break;
			}

			if (io_data->recved >= io_data->max_pkg_len) { // pkg,放不下了,
				int alloc_len = io_data->recved * 2;
				alloc_len = (alloc_len > ((1 << 16) - 1)) ? ((1 << 16) - 1) : alloc_len;

				if (io_data->long_pkg == NULL) { // 小缓存存不下这个命令
					io_data->long_pkg = my_malloc(alloc_len + 1);
					memcpy(io_data->long_pkg, io_data->pkg, io_data->recved);
				}
				else {
					io_data->long_pkg = my_realloc(io_data->long_pkg, alloc_len + 1);
				}
				io_data->max_pkg_len = alloc_len;
			}


			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			unsigned char* buf = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;
			io_data->wsabuffer.buf = buf + io_data->recved;
			io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
			int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}
		// 读到了这个数据,找到了这样的命令, io_data->pkg,开始,  pkg_size
		// end 
		// on_server_recv_line(s, pkg_data, pkg_size);
		on_json_protocal_recv_entry(s, pkg_data, pkg_size);

		if (io_data->recved > pkg_size) {
			memmove(pkg_data, pkg_data + pkg_size, io_data->recved - pkg_size);
		}
		io_data->recved -= pkg_size;
		// printf("=================\n");
		if (io_data->recved == 0) { // IOCP的请求
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;

			}

			io_data->max_pkg_len = MAX_RECV_SIZE;
			io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
			io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

			int ret = WSARecv((SOCKET)s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}
	} // end while
}

static HANDLE g_iocp = NULL;
struct session*
netbus_connect(char* server_ip, int port) {
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		return NULL;
	}
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr(server_ip);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);

	int ret = connect(sock, ((struct sockaddr*) &sockaddr), sizeof(sockaddr));
	if (ret != 0) {
		closesocket(sock);
		return NULL;
	}

	struct session* s = save_session((void*)sock, inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));
	s->socket_type = TCP_SOCKET_IO;
	s->is_server_session = 1; // 它是一个服务器的session
	CreateIoCompletionPort((HANDLE)sock, g_iocp, (DWORD)s, 0);
	post_recv((SOCKET)sock, g_iocp);

	return s;
}

void
start_server(char*ip, int port) {
	int socket_type, protocal_type;

	socket_type = get_socket_type();
	protocal_type = get_proto_type();

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	// 新建一个完成端口;
	SOCKET l_sock = INVALID_SOCKET;
	HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == NULL) {
		goto failed;
	}
	g_iocp = iocp;
	// 创建一个线程
	// CreateThread(NULL, 0, ServerWorkThread, (LPVOID)iocp, 0, 0);
	// end

	// 创建监听socket
	l_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (l_sock == INVALID_SOCKET) {
		goto failed;
	}
	// bind socket
	struct sockaddr_in s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_addr.s_addr = inet_addr(ip);
	s_address.sin_port = htons(port);

	if (bind(l_sock, (struct sockaddr *) &s_address, sizeof(s_address)) != 0) {
		goto failed;
	}

	if (listen(l_sock, SOMAXCONN) != 0) {
		goto failed;
	}

	DWORD dwBytes = 0;
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	WSAIoctl(l_sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx), &lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);

	dwBytes = 0;
	GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	if (0 != WSAIoctl(l_sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptExSockaddrs,
		sizeof(guidGetAcceptExSockaddrs),
		&lpfnGetAcceptExSockaddrs,
		sizeof(lpfnGetAcceptExSockaddrs),
		&dwBytes, NULL, NULL))
	{
	}
	// start 
	CreateIoCompletionPort((HANDLE)l_sock, iocp, (ULONG_PTR)0, 0);
	struct io_package* pkg = my_malloc(sizeof(struct io_package));
	memset(pkg, 0, sizeof(struct io_package));

	post_accept(l_sock, iocp, pkg);
	// end 

	DWORD dwTrans;
	struct session* s;
	//  当我们有完成事件发生了以后,
	// GetQueuedCompletionStatus 会返回我们请求的
	// 时候的WSAOVERLAPPED 的地址,根据这个地址，找到
	// io_data, 找到了io_data,也就意味着我们找到了,
	// 读的缓冲区；
	struct io_package* io_data;

	while (1) {
		// clear_offline_session();
		int m_sec = update_timer_list(GATEWAY_TIMER_LIST);
		// 阻塞函数，当IOCP唤醒这个线程来处理已经发生事件
		// 的时候，才会把这个线程唤醒;
		// IOCP 与select不一样，等待的是一个完成的事件;
		s = NULL;
		dwTrans = 0;
		io_data = NULL;
		int ret = 0;
		if (m_sec > 0) {
			ret = GetQueuedCompletionStatus(iocp, &dwTrans, (PULONG_PTR)&s, (LPOVERLAPPED*)&io_data, m_sec);
		}
		else {
			ret = GetQueuedCompletionStatus(iocp, &dwTrans, (PULONG_PTR)&s, (LPOVERLAPPED*)&io_data, WSA_INFINITE);
		}
		if (ret == 0) {
			if (io_data) {
				if (io_data != NULL && io_data->opt == IOCP_ACCPET) {
					LOGWARNING("error iocp\n");
					closesocket(io_data->accpet_sock);
					post_accept(l_sock, iocp, io_data);
				}
				// 服务器主动关闭socket
				else if (io_data != NULL && io_data->opt == IOCP_RECV) {
					/*DWORD dwRecv = 0;
					DWORD dwFlags = 0;
					if (io_data->long_pkg != NULL) {
						my_free(io_data->long_pkg);
						io_data->long_pkg = NULL;

					}

					io_data->max_pkg_len = MAX_RECV_SIZE;
					io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
					io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

					int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
						1, &dwRecv, &dwFlags,
						&(io_data->overlapped), NULL);*/
					// close_session(s);
					my_free(io_data);
				}
 			}
			continue;
		}
		// IOCP端口唤醒了一个工作线程，
		// 来告诉用户有socket的完成事件发生了;
		// printf("IOCP have event\n");
		if (dwTrans == 0 && io_data->opt == IOCP_RECV) { // socket 关闭
			close_session(s);
			my_free(io_data);
			continue;
		}// end

		switch (io_data->opt) {
			case IOCP_RECV: { // 完成端口意味着数据已经读好
				// 做这个包的完整读取
				io_data->recved += dwTrans; // 表示我们已经读取好的数据的大小;
				if (s->socket_type == TCP_SOCKET_IO) {
					if (protocal_type == BIN_PROTOCAL) {
						on_bin_protocal_recved(s, io_data);
					}
					else if (protocal_type == JSON_PROTOCAL) {
						on_json_protocal_recved(s, io_data);
					}
				}
				else if (s->socket_type == WEB_SOCKET_IO) { // 处理web socket,自己本省就定义了封包与解包的格式;
					if (s->is_shake_hand == 0) { // websocket还没有握手成功,处理握手;
						process_ws_shake_hand(s, io_data, ip, port);
					}
					else { // 处理websocket发送过来的数据包；
						on_ws_pack_recved(s, io_data, protocal_type);
					}
				}
			}
			break;
			case IOCP_ACCPET:
			{
				SOCKET client_fd = io_data->accpet_sock;
				int addr_size = (sizeof(struct sockaddr_in) + 16);
				struct sockaddr_in* l_addr = NULL;
				int l_len = sizeof(struct sockaddr_in);

				struct sockaddr_in* r_addr = NULL;
				int r_len = sizeof(struct sockaddr_in);

				lpfnGetAcceptExSockaddrs(io_data->wsabuffer.buf,
					0, /*io_data->wsabuffer.len - addr_size * 2, */
					addr_size, addr_size,
					(struct sockaddr**)&l_addr, &l_len,
					(struct sockaddr**)&r_addr, &r_len);

				struct session* s = save_session((void*)client_fd, inet_ntoa(r_addr->sin_addr), ntohs(r_addr->sin_port));
				s->socket_type = socket_type;
				CreateIoCompletionPort((HANDLE)client_fd, iocp, (ULONG_PTR)s, 0);
				post_recv(client_fd, iocp);
				post_accept(l_sock, iocp, io_data);
			}
			break;
		}
	}
failed:
	if (iocp != NULL) {
		CloseHandle(iocp);
	}

	if (l_sock != INVALID_SOCKET) {
		closesocket(l_sock);
	}
	
	

	WSACleanup();
}

