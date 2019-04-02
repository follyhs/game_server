#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"
#include "../netbus/netbus.h"
#include "center_services.h"
#include "auth.h"


static void  // 模块初始化入口
init_service_module(struct service_module* module) {

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
		case GUEST_LOGIN:
		{
			guest_login(module_data, json, s, uid, s_key);
		}
		break;
		case EDIT_PROFILE:
		{
			edit_user_profile(module_data, json, s, uid, s_key);
		}
		break;
		case ACCOUNT_UPGRADE:
		{
			guest_upgrade_account(module_data, json, s, uid, s_key);
		}
		break;
		case USER_LOGIN:
		{
			user_login(module_data, json, s, uid, s_key);
		}
		break;
		case MODIFY_UPWD:
		{
			user_modify_upwd(module_data, json, s, uid, s_key);
		}
		break;
	}
	
	return 0;
}

static void
on_connect_lost(void* module_data, struct session* s) {
#ifndef GAME_DEVLOP
	LOGWARNING("gateway lost connected !!!!!");
#endif
}

struct service_module CENTER_SERVICES = {
	STYPE_CENTER,
	init_service_module,
	NULL,
	on_json_protocal_recv,
	on_connect_lost,
	NULL,
};
