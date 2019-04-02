#ifndef __MODEL_LOGIN_BONUES_H__
#define __MODEL_LOGIN_BONUES_H__

#include "../../database/game_database.h"
enum {
	MODEL_COMMON_SUCCESS = 0, 
	MODEL_COMMON_SYSTEM_ERROR = -101, // 系统操作错误
	MODEL_COMMON_NO_LOGIN_BONUES = -102, // 没有登陆奖励
};

int
model_check_login_bonues(unsigned int uid);

int
model_get_login_bonues_info(unsigned int uid, struct login_bonues_info* binfo);


int
model_recv_login_bonues_info(unsigned int uid, struct login_bonues_info* binfo);

#endif

