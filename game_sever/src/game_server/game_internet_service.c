#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"
#include "../netbus/netbus.h"

#include "models/model_internet_server.h"
#include "game_internet_service.h"
#include "internet_server.h"

static void  // 模块初始化入口
init_service_module(struct service_module* module) {
	struct internet_server* s = create_ineternet_server();
	module->module_data = s;
}

static int
on_json_protocal_recv(void* module_data, struct session* s,
json_t* json, unsigned char* data, int len) {
	if (data != NULL) {
		LOGINFO("center_service data = %s", data);
	}
	
	int cmd = json_object_get_number(json, "1");

	// 处理
	unsigned int uid, s_key;
	json_get_command_tag(json, &uid, &s_key);
	// end 

	switch (cmd) {
		case ENTER_SERVER:
		{
			enter_internet_server(module_data, json, s, uid, s_key);
		}
		break;

		case EXIT_SERVER:
		{
			exit_internet_server(module_data, json, s, uid, s_key);
		}
		break;

		case USER_LOST_CONNECT:
		{
			model_exit_internet_server(module_data, uid, 0);
		}
		break;

		case ENTER_ZONE:
		{
			enter_internet_server_zone(module_data, json, s, uid, s_key);
		}
		break;
		case EXIT_ZONE:
		{
			exit_internet_server_zone(module_data, json, s, uid, s_key);
		}
		break;
		case SLICE_FRUIT:
		{
			slice_fruit_internet_server(module_data, json, s, uid, s_key);
		}
		break;
		case PLAYER_SEND_READY:
		{
			player_send_ready_in_room(module_data, json, s, uid, s_key);
		}
		break;
		case PLAYER_SEND_PROP: 
		{
			send_prop_in_room(module_data, json, s, uid, s_key);
		}
		break;
	}
	return 0;
}

static void
on_connect_lost(void* module_data, struct session* s) {
#ifndef GAME_DEVLOP
	LOGWARNING("gateway lost connected !!!!!");
#else // 单进程调试，只用用户丢失
	model_exit_internet_server(module_data, s->uid, 0);
#endif
}

struct service_module GAME_INTERNET_SERVICES = {
	STYPE_GAME_INTERNET,
	init_service_module,
	NULL,
	on_json_protocal_recv,
	on_connect_lost,
	NULL,
};

