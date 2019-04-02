#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../database/game_database.h"
#include "model_mask_and_bonues.h"
#include "../common_config.h"

#include "../../utils/timestamp.h"

int
model_get_mask_and_bonues_info(unsigned int uid,
                               json_t** bonues_array,
							   json_t** mask_array) {

	// 获取已经成但是没有领取的奖励的项
	if (get_user_bonues_info_by_uid(uid, bonues_array) < 0) {
		return MODEL_MASK_AND_BONUES_SYSTEM_ERROR;
	}
	// end 

	// 获取正在进行中的人物
	if (get_user_mask_info_by_uid(uid, mask_array) < 0) {
		return MODEL_MASK_AND_BONUES_SYSTEM_ERROR;
	}
	// end 

	return MODEL_MASK_AND_BONUES_SUCCESS;
}

int
model_winner_task_win_round(unsigned int uid) {
	// 更新数据库，推进赢局的任务向前
	if (update_task_cond(uid, WIND_ROUND_MASK) < 0) {
		return MODEL_MASK_AND_BONUES_SYSTEM_ERROR;
	}
	// end

	// 任务是否达到完成条件
	struct task_cond_info info;
	if (get_task_cond_info(uid, WIND_ROUND_MASK, &info) < 0) {
		return MODEL_MASK_AND_BONUES_SYSTEM_ERROR;
	}
	// end 

	if (!info.has_info) {
		return MODEL_MASK_AND_BONUES_TASK_COMPLETED;
	}

	if (info.now >= info.end_cond) { // 完成任务,任务变成了奖励
		update_task_level_complet(uid, WIND_ROUND_MASK, info.level);
		// 开启下一个等级的任务
		update_task_on_going(uid, WIND_ROUND_MASK, info.level + 1);
	}

	return MODEL_MASK_AND_BONUES_SUCCESS;
}

int
model_recv_task_bonues(unsigned int uid, int task_id, struct task_bonues_info* info) {
	
	if (get_task_bonues_recode_info(uid, task_id, info) < 0) {
		return MODEL_MASK_AND_BONUES_INVALID_OPT;
	}
	if (!info->has_info) {
		return MODEL_MASK_AND_BONUES_INVALID_OPT;
	}

	// 任务奖励的等级, main_level, task_level
	// 将奖励加入到游戏数值里面， uchip 里面
	if (add_ugame_uchip(uid, info->bonues_chip) < 0) {
		return MODEL_MASK_AND_BONUES_SYSTEM_ERROR;
	}
	// end

	// 将奖励的状态标记成以领取
	if (update_task_bonues_status(task_id, 3) < 0) {
		return MODEL_MASK_AND_BONUES_SYSTEM_ERROR;
	}
	// end 

	// 更新reids
	struct ugame_common_info ugame_info;
	get_ugame_common_info_by_uid(uid, &ugame_info);
	set_ugame_common_info_inredis(&ugame_info);
	// end

	// 验证奖励的合法性;
	return MODEL_MASK_AND_BONUES_SUCCESS;
}