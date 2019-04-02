#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "login_model.h"

#include "../../database/center_database.h"

int
model_guest_login(char* ukey, char* unick, 
                  int usex, int uface, struct user_info* uinfo) {
	if (get_uinfo_by_ukey(ukey, uinfo) == 0) {
		if (uinfo->is_guest == 0) { // 这个账号不是游客账号
			return MODEL_ACCOUNT_IS_NOT_GUEST;
		}

		// 刷新到内存里面去;
		set_uinfo_inredis(uinfo);
		return MODEL_SUCCESS;
	}

	// 表示数据库里没有这个用户信息。
	if (insert_guest_with_ukey(ukey, unick, uface, usex) == 0) {
		get_uinfo_by_ukey(ukey, uinfo);
		set_uinfo_inredis(uinfo);
		return MODEL_SUCCESS;
	}
	return MODEL_SYSTEM_ERR;
	// end 
}

int
model_edit_user_profile(unsigned int uid, char* unick, int usex, int uface) {
	if (update_user_profile(uid, unick, uface, usex) != 0) {
		return MODEL_SYSTEM_ERR;
	}

	// 更新成功了，更新redis
	struct user_info uinfo;
	get_uinfo_by_uid(uid, &uinfo);
	set_uinfo_inredis(&uinfo);
	// end 

	return MODEL_SUCCESS;
}

int
model_upgrade_account(unsigned int uid, char* uname, char* upwd_md5) {
	// 验证这个uid是一个游客账号
	struct user_info uinfo;
	if (get_uinfo_by_uid(uid, &uinfo) < 0) {
		return MODEL_INVALID_OPT;
	}

	if (uinfo.is_guest == 0) {
		return MODEL_INVALID_OPT;
	}
	// end 

	if (is_uname_exist(uid, uname) < 0) {
		return MODEL_UNAME_IS_USING;
	}

	if (upgrade_account(uid, uname, upwd_md5) < 0) {
		return MODEL_SYSTEM_ERR;
	}

	// 更新我们的redis
	uinfo.is_guest = 0;
	set_uinfo_inredis(&uinfo);
	// end 

	return MODEL_SUCCESS;
}

int
model_user_login(char* uname, char* upwd, struct user_info* uinfo) {
	if (get_uinfo_by_uname_upwd(uname, upwd, uinfo) < 0) {
		return MODEL_UNAME_OR_UPWD_ERR;
	}

	// 更新我们的redis
	set_uinfo_inredis(uinfo);
	// end 

	return MODEL_SUCCESS;
}

int
model_modify_upwd(unsigned int uid, char* old_upwd_md5, char* new_upwd_md5) {
	// 验证这个uid是一个游客账号
	if (update_user_upwd(uid, old_upwd_md5, new_upwd_md5) != 0) {
		return MODEL_OLD_PWD_ERR;
	}

	return MODEL_SUCCESS;
}

