#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"

#include "ugame_common.h"
#include "models/model_ugame_common.h"
/*
0: 服务号
1: 命令号
返回
status -- OK, err
uchip
uexp
uvip
...
*/

void
get_ugame_common_info(void* module_data, json_t* json,
struct session* s,
	unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_SYSTEM, GET_UGAME_COMMON_INFO, INVALID_PARAMS, uid, s_key);
		return;
	}
	
	struct ugame_common_info info;
	int ret = model_get_ugame_common_info(uid, &info);
	if (ret == MODEL_UGAME_USER_IS_CLOSE_DOWN) {
		write_error(s, STYPE_SYSTEM, GET_UGAME_COMMON_INFO, USER_IS_CLOSE_DOWN, uid, s_key);
		return;
	}

	if (ret != MODEL_UGAME_SUCCESS) {
		write_error(s, STYPE_SYSTEM, GET_UGAME_COMMON_INFO, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// 发送数据会给客户端
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, GET_UGAME_COMMON_INFO, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	json_object_push_number(ret_cmd, "3", info.uchip);
	json_object_push_number(ret_cmd, "4", info.uexp);
	json_object_push_number(ret_cmd, "5", info.uvip);

	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}


void
get_ugame_rank_info(void* module_data, json_t* json,
                    struct session* s,
					unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_SYSTEM, GET_GAME_RANK_INFO, INVALID_PARAMS, uid, s_key);
		return;
	}

	static struct ugame_rank_info rank_info;
	int ret = model_get_ugame_rank_info(uid, &rank_info);
	if (ret != MODEL_UGAME_SUCCESS) {
		write_error(s, STYPE_SYSTEM, GET_GAME_RANK_INFO, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// 发送数据回给客户端
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, GET_GAME_RANK_INFO, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);

	// 排行的数据打包的一个数组里面 [[unick, uface, uchip, usex], [unick, uface, uchip, usex], []]
	json_t* rank_array = json_new_array();
	for (int i = 0; i < rank_info.option_num; i++) { 
		json_t* one_user = json_new_array();
		json_array_push_string(one_user, rank_info.option_set[i].unick);
		json_array_push_number(one_user, rank_info.option_set[i].face);
		json_array_push_number(one_user, rank_info.option_set[i].chip);
		json_array_push_number(one_user, rank_info.option_set[i].sex);

		json_insert_child(rank_array, one_user);
	}
	json_insert_pair_into_object(ret_cmd, "3", rank_array);
	// end 
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}