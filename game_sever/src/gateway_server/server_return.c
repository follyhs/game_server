#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "../netbus/netbus.h"
#include "from_client.h"
#include "server_session.h"
#include "../game_command.h"
#include "../game_result.h"
#include "../game_stype.h"
#include "session_key_man.h"

#define my_malloc malloc

static void
do_relogin(struct session* prev_session) {
	json_t* ret = json_new_command(STYPE_CENTER, RELOGIN);
	session_send_json(prev_session, ret);
	json_free_value(&ret);

	// 关掉以前的session
	close_session(prev_session);
}

static int
on_json_protocal_recv(void* module_data, struct session* s,
json_t* json, unsigned char* data, int len) {
#ifndef GAME_DEVLOP 
	if (!s->is_server_session) {
		return -1;
	}
#endif
	if (data != NULL) {
		LOGINFO("\n========\n %s \n================\n", data);
	}
	
	int stype = (int)(module_data);
	struct session* client_session = NULL;
	// json数据格式里面，必须要携带s_key这个字段
	if (stype == STYPE_CENTER) {
		unsigned int key = (unsigned int)json_object_get_number(json, "s_key");
		client_session = get_session_with_key(key);
		if (client_session) {
			clear_session_with_key(key);
		}
	}
	else {
		unsigned int uid = (unsigned int)json_object_get_number(json, "uid");
		client_session = get_session_with_uid(uid);
	}

	if (client_session == NULL) { // 发送命令的客户端与服务断开了网络连接
		return 0;
	}

	
	// 如果是登陆，并且登陆成功，表示，我们获得了uid,所以，就要将用户的session
	// uid, 赋值为我们登陆成功的uid
	if (stype == STYPE_CENTER) {
		int cmd = json_object_get_number(json, "1");
		if (cmd == GUEST_LOGIN || cmd == USER_LOGIN) {
			// 更新这个uid
			int status = json_object_get_number(json, "2");
			if (status == OK) { // 表示登陆成功
				unsigned int uid = (unsigned int)json_object_get_number(json, "uid");
				client_session->uid = uid;
				struct session* prev_login = get_session_with_uid(uid);
				if (prev_login != NULL) {
					clear_session_with_uid(uid);
					do_relogin(prev_login);
				}
				save_session_with_uid(uid, client_session);
			}
		}
	}
	// 修改，将stype_return,改成stype. 减去STYPE_RETURN这个值
	json_object_update_number(json, "0", stype);
	json_object_remove(json, "s_key");
	json_object_remove(json, "uid");
	// end 

	// 
	// end 
	// 转发给对应的客户端
	session_send_json(client_session, json);
	// end 

	// 封号的逻辑
	if (stype == STYPE_SYSTEM) {
		int cmd = json_object_get_number(json, "1");
		if (cmd == GET_UGAME_COMMON_INFO) {
			int status = json_object_get_number(json, "2");
			if (status == USER_IS_CLOSE_DOWN) {
				clear_session_with_uid(client_session->uid);
				close_session(client_session);
			}
		}
	}
	return 0;
}

static void
on_connect_lost(void* module_data, struct session* s) {
	int stype = (int)(module_data);
	if (s == get_server_session(stype)) {
		disconnect_server(stype);
	}
	
}

void
register_server_return_module(int stype) {
	struct service_module* module = my_malloc(sizeof(struct service_module));
	memset(module, 0, sizeof(struct service_module));

	module->stype = stype;
	module->init_service_module = NULL;
	module->on_bin_protocal_recv = NULL;
	module->on_json_protocal_recv = on_json_protocal_recv;
	module->on_connect_lost = on_connect_lost;
	module->module_data = (void*)stype;

	register_service(stype + STYPE_RETURN, module);
}