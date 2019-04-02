#ifndef __SERVER_NETBUS_H__
#define __SERVER_NETBUS_H__

#include "socket/session.h"
#include "../utils/log.h"
#include "../3rd/mjson/json.h"



enum {
	TCP_SOCKET_IO = 0,  // tcp socket
	WEB_SOCKET_IO = 1,  // websocket
};

enum {
	BIN_PROTOCAL = 0, // 二进制协议
	JSON_PROTOCAL = 1, // json协议
};

void
init_server_netbus(int socket_type, int protocal_type);

void
exit_server_netbus();

void start_server(char* ip, int port);


struct service_module{
	int stype; // 服务的类型，系统根据这个服务的类型来讲消息分发给对应的服务

	void  // 模块初始化入口
	(*init_service_module)(struct service_module* module);

	int  // 如果不为0，底层会关闭掉这个socket;
	(*on_bin_protocal_recv)(void* module_data, struct session* s, 
	                        unsigned char* data, int len); 

	int
	(*on_json_protocal_recv)(void* module_data, struct session* s,
	                         json_t* json, unsigned char* data, int len);

	void
	(*on_connect_lost)(void* module_data, struct session* s); // 连接丢失，收到这个函数;
	
	void* module_data; // 用来携带service_module用户自定义数据的;
};

// 注册服务的接口函数;
void 
register_service(int stype, struct service_module* module);

// 用户不用管地下的分包，只管发送数据就可以了；
void 
session_send(struct session*s, unsigned char* body, int len);

void 
session_send_json(struct session* s, json_t* json);
// end 

// 无需要释放这个cmd
void 
session_send_json_cmd_to_service(struct session* s, int to_stype, json_t* cmd);

unsigned int
netbus_add_timer(void(*on_timer)(void* udata),
                  void* udata, float after_sec);

void
netbus_cancel_timer(unsigned int timeid);

unsigned int
netbus_schedule(void(*on_timer)(void* udata),
void* udata, float after_sec);

struct session*
netbus_connect(char* server_ip, int port);


#endif


