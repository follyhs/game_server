#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"

#include "login_bonueses.h"

#include "models/model_login_bonues.h"
/*
服务号
命令号
返回
0:服务号
1:命令号
2:状态码
status OK, 
*/

void
check_login_bonues(void* module_data, json_t* json,
                   struct session* s,
				   unsigned int uid, unsigned int s_key) {
	// step1 检查命令的格式长度
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_SYSTEM, CHECK_LOGIN_BONUES, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end 

	int ret = model_check_login_bonues(uid);
	if (ret != MODEL_COMMON_SUCCESS) {
		write_error(s, STYPE_SYSTEM, CHECK_LOGIN_BONUES, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// 检查登陆奖励成功
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, CHECK_LOGIN_BONUES, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}

				   /*
服务号
命令号
返回
0:服务号
1:命令号
2:状态码
status OK, 
bonus_value: // 没有领取登陆奖励
udays, // 连续登陆的天数
*/
void 
get_login_bonues_info(void* module_data, json_t* json,
struct session* s,
	unsigned int uid, unsigned int s_key) {

	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_SYSTEM, GET_LOGIN_BONUSES_INFO, INVALID_PARAMS, uid, s_key);
		return;
	}

	struct login_bonues_info binfo;
	int ret = model_get_login_bonues_info(uid, &binfo);
	if (ret == MODEL_COMMON_NO_LOGIN_BONUES) {
		write_error(s, STYPE_SYSTEM, GET_LOGIN_BONUSES_INFO, NO_LOGIN_BONUES, uid, s_key);
		return;
	}

	if (ret == MODEL_COMMON_SYSTEM_ERROR) {
		write_error(s, STYPE_SYSTEM, GET_LOGIN_BONUSES_INFO, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// 获取登陆奖励成功
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, GET_LOGIN_BONUSES_INFO, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	json_object_push_number(ret_cmd, "3", binfo.bonues_value);
	json_object_push_number(ret_cmd, "4", binfo.udays);

	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}

/*
服务号
命令号
返回
0:服务号
1:命令号
2:状态码
status OK,
bonus_value: 领取奖励的数目
*/
void
recv_login_bonues(void* module_data, json_t* json,
                  struct session* s,
				  unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_SYSTEM, RECV_LOGIN_BONUSES, INVALID_PARAMS, uid, s_key);
		return;
	}

	struct login_bonues_info binfo;
	int ret = model_recv_login_bonues_info(uid, &binfo);
	if (ret == MODEL_COMMON_NO_LOGIN_BONUES) {
		write_error(s, STYPE_SYSTEM, RECV_LOGIN_BONUSES, NO_LOGIN_BONUES, uid, s_key);
		return;
	}

	if (ret != MODEL_COMMON_SUCCESS) {
		write_error(s, STYPE_SYSTEM, RECV_LOGIN_BONUSES, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// 获取登陆奖励成功
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, RECV_LOGIN_BONUSES, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	json_object_push_number(ret_cmd, "3", binfo.bonues_value);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
}