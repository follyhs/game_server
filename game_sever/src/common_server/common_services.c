#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"
#include "../netbus/netbus.h"
#include "common_services.h"
#include "login_bonueses.h"
#include "ugame_common.h"
#include "mask_and_bonues.h"


static void  // 模块初始化入口
init_service_module(struct service_module* module) {

}

// 系统通用服务的命令入口
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

	// 这个uid是网关服务器给我们的，所以它一定是可信的。
	if (uid == 0) {
		return 0;
	}

	switch (cmd) {
		// 用户登录成功，是否有登陆奖励
		case CHECK_LOGIN_BONUES:
		{
			check_login_bonues(module_data, json, s, uid, s_key);
		}
		break;
		case GET_LOGIN_BONUSES_INFO: 
		{
			get_login_bonues_info(module_data, json, s, uid, s_key);
		}
		break;
		case RECV_LOGIN_BONUSES:
		{
			recv_login_bonues(module_data, json, s, uid, s_key);
		}
		break;
		case RECV_TASK_BONUES:
		{
			recv_task_bonues(module_data, json, s, uid, s_key);
		}
		break;
		case GET_UGAME_COMMON_INFO:
		{
			get_ugame_common_info(module_data, json, s, uid, s_key);
		}
		break;
		case GET_GAME_RANK_INFO: 
		{
			get_ugame_rank_info(module_data, json, s, uid, s_key);
		}
		break;

		case GET_MASK_AND_BONUES_INFO: {
			get_mask_and_bonues_info(module_data, json, s, uid, s_key);
		}
		break;
		// 玩家赢了一场，来推动任务的向前发展
		case TASK_WIN_EVENT: 
		{
			task_and_bonues_win_one_round(module_data, json, s, uid, s_key);
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

struct service_module COMMON_SERVICES = {
	STYPE_SYSTEM,
	init_service_module,
	NULL,
	on_json_protocal_recv,
	on_connect_lost,
	NULL,
};

