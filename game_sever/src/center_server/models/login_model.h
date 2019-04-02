#ifndef __LOGION_MODEL_H__
#define __LOGION_MODEL_H__

#include "../../database/center_database.h"

// 返回用户的uid
enum {
	MODEL_SUCCESS = 0, // 操作成功
	MODEL_ERROR = -1, // 通用的error
	MODEL_ACCOUNT_IS_NOT_GUEST = -2, // 账号不是游客账号
	MODEL_SYSTEM_ERR = -3, // 系统错误
	MODEL_INVALID_OPT = -4, // 非法的操作
	MODEL_UNAME_IS_USING = -5, // 名字被其他的人使用
	MODEL_UNAME_OR_UPWD_ERR = -6, // 用户名或密码错误,
	MODEL_OLD_PWD_ERR = -7, // 旧的密码不对
};

int 
model_guest_login(char* ukey, char* unick, int usex, int uface, struct user_info* out_info);

int
model_user_login(char* uname, char* upwd, struct user_info* out_info);

int
model_edit_user_profile(unsigned int uid, char* unick, int usex, int uface);

int
model_upgrade_account(unsigned int uid, char* uname, char* upwd_md5);

int
model_modify_upwd(unsigned int uid, char* old_upwd_md5, char* new_upwd_md5);

#endif



