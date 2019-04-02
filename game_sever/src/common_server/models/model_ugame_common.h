#ifndef __MODEL_UGAME_COMMON_H__
#define __MODEL_UGAME_COMMON_H__

#include "../../database/game_database.h"

enum {
	MODEL_UGAME_SUCCESS = 0,
	MODEL_UGAME_SYSTEM_ERR = -102,
	MODEL_UGAME_USER_IS_NOT_EXIST = -103,
	MODEL_UGAME_USER_IS_CLOSE_DOWN = -104, // 用户已经被分号
};



int
model_get_ugame_common_info(unsigned int uid, struct ugame_common_info* info);

int
model_get_ugame_rank_info(unsigned int uid, struct ugame_rank_info* rank_info);
#endif

