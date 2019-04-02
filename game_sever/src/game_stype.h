#ifndef __GAME_STYPE_H__
#define __GAME_STYPE_H__

enum {
	STYPE_SERVER_POST = 1,
	STYPE_CENTER = 2, // 用户数据，登陆，资料服务
	STYPE_SYSTEM = 3, // 任务，邮件，奖励，排行
	STYPE_GAME_INTERNET = 4, // 网络对战
	STYPE_GAME_FRIEND = 5, // 好友对战
};

// 这个框架允许最多是512个服务模块[0, 255],[256, 511]
#ifdef GAME_DEVLOP
#define STYPE_RETURN 0
#else
#define STYPE_RETURN 0xff
#endif

// return的

#endif

