#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../netbus/netbus.h"
#include "from_client.h"
#include "server_session.h"
#include "../game_command.h"
#include "session_key_man.h"
#include "../game_stype.h"

#define my_malloc malloc

static int
on_json_protocal_recv(void* module_data, struct session* s,
json_t* json, unsigned char* data, int len) {
	int stype = (int)(module_data);
	struct session* server_session = get_server_session(stype);
	if (server_session == NULL) { // gateway与服务所在进程断开了网络连接
		// 回一个客户端网络异常的错误
		// end 
		return 0;
	}
	// 验证,这个用户已经登陆过l
	unsigned int uid = s->uid; // 通过session来获得uid,
	// end 

	// 中心服务除外，所有的服务都要求必须登陆
	if (stype != STYPE_CENTER && uid == 0) {
		return 0;
	}

	// 转发给对应的服务器
	// token_key, 方便我们找到client_session
	json_object_push_number(json, "uid", uid);

	unsigned int key;
	if (stype == STYPE_CENTER) {
		key = get_key_from_session_map();
		save_session_with_key(key, s);
	}
	else {
		key = 0;
	}
	
	// s->s_key = key;

	json_object_push_number(json, "s_key", key);

	session_send_json(server_session, json);
	// end 
	return 0;
}

static void
on_connect_lost(void* module_data, struct session* s) {
	if (s->is_server_session) {
		return;
	}

	clear_session_with_value(s);

	unsigned int uid = s->uid;
	if (uid == 0) { // 没有登陆，或登陆不成功
		return;
	}

	int stype = (int)(module_data);
	if (stype == STYPE_CENTER) {
		clear_session_with_uid(uid);
	}

	
	// 通知这个服务器，用户掉线了
	json_t* json = json_new_command(stype, USER_LOST_CONNECT);

	
	json_object_push_number(json, "2", uid);

	json_object_push_number(json, "uid", uid);
	json_object_push_number(json, "s_key", 0);
	struct session* server_session = get_server_session(stype);
	if (server_session == NULL) { // gateway与服务所在进程断开了网络连接
		json_free_value(&json);
		return;
	}
	session_send_json(server_session, json);
	json_free_value(&json);
	// end 
	// clear_session_with_key(s->s_key);

} 

void 
register_from_client_module(int stype) {
	struct service_module* module = my_malloc(sizeof(struct service_module));
	memset(module, 0, sizeof(struct service_module));

	module->stype = stype;
	module->init_service_module = NULL;
	module->on_bin_protocal_recv = NULL;
	module->on_json_protocal_recv = on_json_protocal_recv;
	module->on_connect_lost = on_connect_lost;
	module->module_data = (void*) stype;

	register_service(stype, module);
}