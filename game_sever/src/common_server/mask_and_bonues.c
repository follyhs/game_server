#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"

#include "ugame_common.h"
#include "models/model_ugame_common.h"
#include "models/model_mask_and_bonues.h"
/*
0: 服务号
1: 命令号
返回
2: status -- OK
3: [已经完成，但是没有领取的奖励]
4: [正在完成的任务]
... 赞不开考虑
5: [未开始的任务]
6: [已经完成，已经领取的奖励]
...
*/

void
get_mask_and_bonues_info(void* module_data, json_t* json,
                        struct session* s,
                        unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_SYSTEM, GET_MASK_AND_BONUES_INFO, INVALID_PARAMS, uid, s_key);
		return;
	}

	json_t* bonus_array = NULL;
	json_t* mask_array = NULL;

	int ret = model_get_mask_and_bonues_info(uid, &bonus_array, &mask_array);
	if (ret != MODEL_MASK_AND_BONUES_SUCCESS) {
		write_error(s, STYPE_SYSTEM, GET_MASK_AND_BONUES_INFO, INVALID_PARAMS, uid, s_key);
		return;
	}


	// mask bonues发送给我们客户端
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, GET_MASK_AND_BONUES_INFO, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	json_insert_pair_into_object(ret_cmd, "3", bonus_array);
	json_insert_pair_into_object(ret_cmd, "4", mask_array);

	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
	// end 
}

void
task_and_bonues_win_one_round(void* module_data, json_t* json,
                              struct session* s,
                              unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 2 + 2) {
		return;
	}

	// uid 赢了一局，然后，推动这个玩家的任务像前
	model_winner_task_win_round(uid);
	// end 
}

/*
0: 服务号
1: 命令号
2: task_id
返回:
0: 服务号
1: 命令号
2: 状态码 --OK
3: 奖励1
4: 奖励2..
*/

void 
recv_task_bonues(void* module_data, json_t* json,
                 struct session* s,
				  unsigned int uid, unsigned int s_key) {
	
	int len = json_object_size(json);
	if (len != 3 + 2) {
		write_error(s, STYPE_SYSTEM, RECV_TASK_BONUES, INVALID_PARAMS, uid, s_key);
		return;
	}

	int task_id = json_object_get_number(json, "2");
	if (task_id <= 0) {
		write_error(s, STYPE_SYSTEM, RECV_TASK_BONUES, INVALID_PARAMS, uid, s_key);
		return;
	}

	struct task_bonues_info info;
	int ret = model_recv_task_bonues(uid, task_id, &info);
	if (ret != MODEL_MASK_AND_BONUES_SUCCESS) {
		write_error(s, STYPE_SYSTEM, RECV_TASK_BONUES, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// ok 
	json_t*  ret_cmd = json_new_server_return_cmd(STYPE_SYSTEM, RECV_TASK_BONUES, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	json_object_push_number(ret_cmd, "3", info.bonues_chip);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end
}
