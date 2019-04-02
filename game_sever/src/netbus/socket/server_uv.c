#include <uv.h>
#include <stdio.h>
#include <stdlib.h>

static uv_loop_t* loop;
static uv_tcp_t tcpServer;
static uv_handle_t* server;
static uv_connect_t* connect_req;

static char ip_address[64];
static int ip_port;

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "libuv.lib")
#endif

#define my_malloc malloc
#define my_free free
#define my_realloc realloc

#define MAX_PKG_SIZE ((1<<16) - 1)
#define MAX_RECV_SIZE 2048


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

void
close_session(struct session* s);

struct io_package {
	struct session* s;
	int recved; // 收到的字节数;
	unsigned char* long_pkg;
	int max_pkg_len;
};

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

static void
after_write(uv_write_t* req, int status) {
	write_req_t* wr;

	/* Free the read/write buffer and the request */
	wr = (write_req_t*)req;
	my_free(wr->buf.base);
	my_free(wr);

	if (status == 0)
		return;

	fprintf(stderr,
		"uv_write error: %s - %s\n",
		uv_err_name(status),
		uv_strerror(status));
}

void
socket_send_data(void* ud, unsigned char* buf, int nread) {
	uv_stream_t* handle = (uv_stream_t*)ud;
	write_req_t *wr;
	wr = (write_req_t*)my_malloc(sizeof(write_req_t));
	
	int alloc_size = (nread < 2048) ? 2048 : nread;
	unsigned char* send_buf = my_malloc(alloc_size);
	memcpy(send_buf, buf, nread);
	wr->buf = uv_buf_init(send_buf, nread);

	if (uv_write(&wr->req, handle, &wr->buf, 1, after_write)) {
		LOGERROR("uv_write failed");
	}
}

static void
uv_close_stream(uv_handle_t* peer) {
	struct io_package* io_data = peer->data;
	if (io_data->s != NULL) {
		io_data->s->c_sock = NULL;
		remove_session(io_data->s);
		io_data->s = NULL;
	}

	if (peer->data != NULL) {
		my_free(peer->data);
		peer->data = NULL;
	}

	my_free(peer);
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
	unsigned char* data = io_data->long_pkg;
	http_parser_execute(&p, &s, data, io_data->recved);

	// 如果没有拿到key_migic,表示数据没有收完，我们去收握手的数据;
	if (has_client_key == 0) {
		ses->is_shake_hand = 0;
		if (io_data->recved >= MAX_RECV_SIZE) { // 不正常的握手包;
			close_session(ses->c_sock);
			// uv_close(ses->c_sock, uv_close_stream);
			return -1;
		}
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
	socket_send_data(ses->c_sock, accept_buffer, (int)strlen(accept_buffer));
	ses->is_shake_hand = 1;
	base64_encode_free(b64_str);

	if (io_data->long_pkg != NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;

	}
	io_data->recved = 0;
	io_data->max_pkg_len = 0;
	
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
		// len = ((pkg_data[2]) | (pkg_data[3] << 8));
		len = ((pkg_data[3]) | (pkg_data[2] << 8));
		mask = pkg_data + 2 + 2;
	}
	// 1 + 1 + 8(长度信息) + 4MASK
	else if (len == 127){ // 这种情况不用考虑,考虑前4个字节的大小，后面不管;
		if (pkg_len < 14) {
			return -1;
		}
		unsigned int low = ((pkg_data[5]) | (pkg_data[4] << 8) | (pkg_data[3] << 16) | (pkg_data[2] << 24));
		unsigned int hight = ((pkg_data[9]) | (pkg_data[8] << 8) | (pkg_data[7] << 16) | (pkg_data[6] << 24));
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
		unsigned char* pkg_data = io_data->long_pkg;

		if (recv_ws_header(pkg_data, io_data->recved, &pkg_size, &header_size) != 0) { // 继续投递recv请求，知道能否接收一个数据头;
			break;
		}

		// Step2:判断数据大小，是否不符合规定的格式
		if (pkg_size >= MAX_PKG_SIZE) { // ,异常的数据包，直接关闭掉socket;
			// uv_close(s->c_sock, uv_close_stream);
			close_session(s);
			break;
		}

		// 是否收完了一个数据包;
		if (io_data->recved >= pkg_size) { // 表示我们已经收到至少超过了一个包的数据；
			// 0x81
			if (pkg_data[0] == 0x88) { // 对方要关闭socket
				// uv_close(s->c_sock, uv_close_stream);
				close_session(s);
				break;
			}

			parser_ws_pack(s, pkg_data + header_size,
				pkg_size - header_size, pkg_data + header_size - 4, protocal_type);

			if (io_data->recved > pkg_size) { // 1.5 个包
				memmove(io_data->long_pkg, io_data->long_pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->recved == 0 && io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
				io_data->max_pkg_len = 0;
			}
			break;
		}
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
		if (recv_header(io_data->long_pkg, io_data->recved, &pkg_size) != 0) { // 继续投递recv请求，知道能否接收一个数据头;
			break;
		}

		// Step2:判断数据大小，是否不符合规定的格式
		if (pkg_size >= MAX_PKG_SIZE) { // ,异常的数据包，直接关闭掉socket;
			// uv_close(s->c_sock, uv_close_stream);
			close_session(s->c_sock);
			break;
		}

		// 是否收完了一个数据包;
		if (io_data->recved >= pkg_size) { // 表示我们已经收到至少超过了一个包的数据；
			unsigned char* pkg_data = io_data->long_pkg;

			// on_server_recv(s, pkg_data + 2, pkg_size - 2);
			on_bin_protocal_recv_entry(s, pkg_data + 2, pkg_size - 2);

			if (io_data->recved > pkg_size) { // 1.5 个包
				memmove(io_data->long_pkg, io_data->long_pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->recved == 0 && io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
				io_data->max_pkg_len = 0;
			}
		}
		// end 
	}
}

static void
on_json_protocal_recved(struct session* s, struct io_package* io_data) {
	while (io_data->recved > 0) {
		int pkg_size = 0;
		unsigned char* pkg_data = io_data->long_pkg;

		if (read_pkg_tail(pkg_data, io_data->recved, &pkg_size) != 0) { // 没有读到\r\n
			if (io_data->recved >= (((1 << 16) - 1))) { // 超过了数据包,close session
				// uv_close(s->c_sock, uv_close_stream);
				close_session(s->c_sock);
			}
			break;
		}
		// 读到了这个数据,找到了这样的命令, io_data->pkg,开始,  pkg_size
		// end 
		// on_server_recv_line(s, pkg_data, pkg_size);
		on_json_protocal_recv_entry(s, pkg_data, pkg_size);

		if (io_data->recved > pkg_size) {
			memmove(io_data->long_pkg, io_data->long_pkg + pkg_size, io_data->recved - pkg_size);
		}
		io_data->recved -= pkg_size;
		if (io_data->recved == 0 && io_data->long_pkg != NULL) {
			my_free(io_data->long_pkg);
			io_data->long_pkg = NULL;
			io_data->max_pkg_len = 0;
		}
	} // end while
}

static void
uv_buf_alloc(uv_handle_t* handle,
size_t suggested_size,
uv_buf_t* buf) {
	struct io_package* io_data = (struct io_package*)handle->data;
	int alloc_len = (io_data->recved + suggested_size);
	alloc_len = (alloc_len > ((1 << 16) - 1)) ? ((1 << 16) - 1) : alloc_len;
	if (alloc_len < MAX_RECV_SIZE) {
		alloc_len = MAX_RECV_SIZE;
	}

	if ((alloc_len) > io_data->max_pkg_len) { // pkg,放不下了,
		io_data->long_pkg = my_realloc(io_data->long_pkg, alloc_len + 1);
		io_data->max_pkg_len = alloc_len;
	}
	unsigned char* data = io_data->long_pkg;
	buf->base = data + io_data->recved;
	buf->len = suggested_size;
}

static void 
after_shutdown(uv_shutdown_t* req, int status) {
	uv_close((uv_handle_t*)req->handle, uv_close_stream);
	my_free(req);
}

static void
my_add_timer(int msec);
static uv_timer_t timer_req;
static void timer_callback(uv_timer_t *handle) {
	int m_sec = update_timer_list(GATEWAY_TIMER_LIST);
	if (m_sec > 0) {
		my_add_timer(m_sec);
	}
}

static void
my_add_timer(int msec) {
	uv_timer_start(&timer_req, timer_callback, msec, 1);
}

static void after_read(uv_stream_t* handle,
	ssize_t nread,
	const uv_buf_t* buf) {

	uv_shutdown_t* sreq;

	if (nread < 0) {
		/* Error or EOF */
		// assert(nread == UV_EOF);
		sreq = my_malloc(sizeof(uv_shutdown_t));
		uv_shutdown(sreq, handle, after_shutdown);
		return;
	}

	if (nread == 0) {
		/* Everything OK, but nothing read. */
		// my_free(buf->base);
		return;
	}

	struct io_package* io_data = handle->data;
	if (io_data) {
		io_data->recved += nread;
	}

	int protocal_type = get_proto_type();
	struct session* s = io_data->s;

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
			process_ws_shake_hand(s, io_data, ip_address, ip_port);
		}
		else { // 处理websocket发送过来的数据包；
			on_ws_pack_recved(s, io_data, protocal_type);
		}
	}

	// cancel timer adjust timer
	int m_sec = update_timer_list(GATEWAY_TIMER_LIST);
	if (m_sec > 0) {
		my_add_timer(m_sec);
	}
	// end 
}

static void on_connection(uv_stream_t* server, int status) {
	uv_stream_t* stream;
	int r;

	if (status != 0) {
		LOGERROR("Connect error %s\n", uv_err_name(status));
		return;
	}
	
	stream = my_malloc(sizeof(uv_tcp_t));
	
	r = uv_tcp_init(loop, (uv_tcp_t*)stream);
	r = uv_accept(server, stream);
	r = uv_read_start(stream, uv_buf_alloc, after_read);

	struct io_package* io_data;
	io_data = my_malloc(sizeof(struct io_package));
	stream->data = io_data;
	io_data->max_pkg_len = MAX_RECV_SIZE;
	memset(stream->data, 0, sizeof(struct io_package));

	struct session* s = save_session(stream, "127.0.0.1", 100);
	io_data->s = s;
	io_data->long_pkg = NULL;

	s->socket_type = get_socket_type();
}

static int tcp4_server_start(uv_loop_t* loop, int port) {
	struct sockaddr_in addr;
	int r;

	uv_ip4_addr("0.0.0.0", port, &addr);

	server = (uv_handle_t*)&tcpServer;

	r = uv_tcp_init(loop, &tcpServer);
	if (r) {
		/* TODO: Error codes */
		LOGERROR("Socket creation error\n");
		return 1;
	}

	r = uv_tcp_bind(&tcpServer, (const struct sockaddr*) &addr, 0);
	if (r) {
		/* TODO: Error codes */
		LOGERROR("Bind error\n");
		return 1;
	}

	r = uv_listen((uv_stream_t*)&tcpServer, SOMAXCONN, on_connection);
	if (r) {
		/* TODO: Error codes */
		LOGERROR("Listen error %s\n", uv_err_name(r));
		return 1;
	}

	return 0;
}

void
start_server(char*ip, int port) {
	int socket_type, protocal_type;

	socket_type = get_socket_type();
	protocal_type = get_proto_type();
	strcpy(ip_address, ip);
	ip_port = port;

	loop = uv_default_loop();
	uv_timer_init(uv_default_loop(), &timer_req);
	int msec = update_timer_list(GATEWAY_TIMER_LIST);
	if (msec > 0) {
		my_add_timer(msec);
	}
	if (tcp4_server_start(loop, port)) {
		return;
	}

	uv_run(loop, UV_RUN_DEFAULT);
}

static void
after_connect(uv_connect_t* handle, int status) {
	if (status) {
		LOGERROR("connect error");
		uv_close((uv_handle_t*)handle->handle, uv_close_stream);
		return;
	}

	int iret = uv_read_start(handle->handle, uv_buf_alloc, after_read);
	if (iret) {
		LOGERROR("uv_read_start error");
		return;
	}
}

struct session*
netbus_connect(char* server_ip, int port) {

	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(server_ip, port, &bind_addr);
	if (iret) {
		return NULL;
	}
	
	uv_tcp_t* stream = my_malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, stream);

	struct io_package* io_data;
	io_data = my_malloc(sizeof(struct io_package));
	memset(io_data, 0, sizeof(struct io_package));
	stream->data = io_data;
	
	struct session* s = save_session(stream, server_ip, port);
	io_data->max_pkg_len = 0;
	io_data->s = s;
	io_data->long_pkg = NULL;
	s->socket_type = TCP_SOCKET_IO;
	s->is_server_session = 1;

	connect_req = my_malloc(sizeof(uv_connect_t));
	iret = uv_tcp_connect(connect_req, stream, (struct sockaddr*)&bind_addr, after_connect);
	if (iret) {
		LOGERROR("uv_tcp_connect error!!!");
		return NULL;
	}

	return s;
}

void
close_session(struct session* s) {
	if (s->c_sock) {
		uv_shutdown_t* sreq = my_malloc(sizeof(uv_shutdown_t));
		uv_shutdown(sreq, s->c_sock, after_shutdown);
	}
}
