#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"
#include "../netbus/netbus.h"
#include "../game_stype.h"
#include "./server_session.h"

#include "server_post.h"


static void  // 模块初始化入口
init_service_module(struct service_module* module) {

}

static int
on_json_protocal_recv(void* module_data, struct session* s,
json_t* json, unsigned char* data, int len) {
	if (!s->is_server_session) {
		return 0;
	}

	if (data != NULL) {
		LOGINFO("server_post data = %s", data);
	}

	json_t* cmd = json_object_at(json, "2");
	if (!cmd) {
		return 0;
	}

	int stype = json_object_get_number(cmd, "0");
	stype -= STYPE_RETURN;

	struct session* server_session = get_server_session(stype);
	if (server_session == NULL) { // gateway与服务所在进程断开了网络连接
		// 回一个客户端网络异常的错误
		// end 
		return 0;
	}
	json_object_update_number(cmd, "0", stype);
	session_send_json(server_session, cmd);

	return 0;
}

static void
on_connect_lost(void* module_data, struct session* s) {
}

struct service_module SERVER_POST_SERVICE = {
	STYPE_SERVER_POST,
	init_service_module,
	NULL,
	on_json_protocal_recv,
	on_connect_lost,
	NULL,
};
