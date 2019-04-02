#ifndef __GAME_RESULT_H__
#define __GAME_RESULT_H__
// respones status code
enum {
	OK = 1, // 成功
	INVALID_CMD = -100, // 不支持的命令号
	INVALID_PARAMS = -101, // 参数错误
	ACCOUNT_IS_NOT_GUEST = -102, // 不是游客账号
	SYSTEM_ERROR = -103, // 系统错误
	INVALID_OPT = -104, // 非法的操作
	UNAME_IS_USING = -105, // 用户名已经占用，不能升级
	UNAME_OR_UPWD_ERROR = -106, // 用户名或密码错误
	OLD_UPWD_ERROR = -106, // 旧的密码错误
	NO_LOGIN_BONUES = -107, // 没有登陆奖励
	USER_IS_CLOSE_DOWN = -108, // 用户已经被封号
	USER_IS_NOT_IN_SEVER = -109, // 用户不在服务器上.
	USER_IS_IN_ZONE = -110, // 用户已经在这个区间
	INVALIDI_ZOOM_ID = -111, // 错误的用户id号
	USER_IS_NOT_IN_ZONE = -112, // 用户不在这个区间
	USER_IS_IN_GAME = -113, // 用户在游戏中
};

#endif

