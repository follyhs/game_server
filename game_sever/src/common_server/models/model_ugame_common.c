#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "model_ugame_common.h"
#include "../common_config.h"

#include "../../database/center_database.h"
#include "../../database/game_database.h"


int
model_get_ugame_common_info(unsigned int uid, struct ugame_common_info* info) {
	// 查询这个游戏的游戏信息
	if (get_ugame_common_info_by_uid(uid, info) == 0) {
		set_ugame_common_info_inredis(info);
		if (info->ustatus != 0) { // 已经封号,所以获取信息失败
			return MODEL_UGAME_USER_IS_CLOSE_DOWN;
		}

		// 插入我们的更新的任务
		if (insert_game_task(uid) < 0) {
			return MODEL_UGAME_SYSTEM_ERR;
		}
		// end 

		// 刷新一下排行榜
		flush_game_rank_info_inredis(info);
		// end 

		return MODEL_UGAME_SUCCESS;
	}

	// 插入新的
	info->uchip = COMMON_CONF.uchip;
	info->uexp = COMMON_CONF.uexp;
	info->uvip = COMMON_CONF.uvip; 
	info->ustatus = 0;
	info->uid = uid;

	if (insert_ugame_common_info_by_uid(uid, info) < 0) {
		return MODEL_UGAME_SYSTEM_ERR;
	}

	// 插入我们的任务
	if (insert_game_task(uid) < 0) {
		return MODEL_UGAME_SYSTEM_ERR;
	}
	// end 

	// end 

	// 写入到redis
	set_ugame_common_info_inredis(info);
	// end 
	
	// 刷新一下排行榜
	flush_game_rank_info_inredis(info);
	// end 

	

	return MODEL_UGAME_SUCCESS;
	// end 
	
}

int
model_get_ugame_rank_info(unsigned int uid, struct ugame_rank_info* rank_info) {
	struct game_rank_user rank_user;
	if (get_game_rank_user_inredis(&rank_user) != 0) {
		return MODEL_UGAME_SYSTEM_ERR;
	}

	memset(rank_info, 0, sizeof(struct ugame_rank_info));
	rank_info->option_num = rank_user.rank_num;

	// for uid
	for (int i = 0; i < rank_user.rank_num; i++) {

		struct user_info uinfo;
		get_uinfo_inredis(rank_user.rank_user_uid[i], &uinfo);

		struct ugame_common_info ugame_info;
		get_ugame_common_info_by_uid(rank_user.rank_user_uid[i], &ugame_info);

		rank_info->option_set[i].chip = ugame_info.uchip;
		rank_info->option_set[i].face = uinfo.uface;
		strcpy(rank_info->option_set[i].unick, uinfo.unick);
		rank_info->option_set[i].sex = uinfo.usex;
	}
	// end 
	return MODEL_UGAME_SUCCESS;
}

