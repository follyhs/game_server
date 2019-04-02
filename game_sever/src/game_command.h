#ifndef __GAME_COMMAND_H__
#define __GAME_COMMAND_H__

struct session;

#include "3rd/mjson/json.h"
void
json_get_command_tag(json_t* cmd, unsigned int* uid, unsigned int* s_key);

json_t*
json_new_server_return_cmd(int stype, int cmd,
                      unsigned int uid,
					  unsigned int s_key);

void
write_error(struct session* s, int stype,
            int cmd, int status,
			unsigned int uid, unsigned int s_key);

// 所有的服务共享命令
enum {
	USER_LOST_CONNECT = 1,
	COMMON_NUM,
};

// SYPTE_CENTER
enum {
	GUEST_LOGIN = COMMON_NUM, // 游客登陆
	USER_LOGIN, // 用户密码登录
	EDIT_PROFILE, // 修改用户资料
	RELOGIN, // 用户在新的地方登陆
	ACCOUNT_UPGRADE, // 账号升级
	MODIFY_UPWD, // 修改密码
};

// STYPE_SYSTEM, 邮件，排行, 任务，奖励
enum {
	CHECK_LOGIN_BONUES = COMMON_NUM, // 发送登陆成功的消息给我们系统服务器。
	GET_LOGIN_BONUSES_INFO = 3, // 获取登陆奖励
	RECV_LOGIN_BONUSES = 4, // 领取登陆奖励 
	GET_UGAME_COMMON_INFO = 5, // 获取玩家的公共信息，金币，VIP, 背包, 钻石等
	GET_GAME_RANK_INFO = 6, // 获取游戏排行榜
	GET_MASK_AND_BONUES_INFO = 7, // 拉取我们的任务和奖励
	TASK_WIN_EVENT = 8, // 有一个玩家赢了一场比赛
	RECV_TASK_BONUES = 9, // 领取我们的任务奖励
};

// STYPE_GAME_INTERNET 
// 网络对战服务
enum {
	ENTER_SERVER = COMMON_NUM, // 进入服务器
	EXIT_SERVER = 3, // 离开服务器

	ENTER_ZONE = 4, // 进入到哪个区
	EXIT_ZONE = 5, // 离开哪个区

	ENTER_ROOM = 6, // 进入房间
	EXIT_ROOM = 7, // 离开房间
	USER_ARRIVED = 8, // 有其他的用户进来。
	USER_STANDUP = 9, // 玩家站起

	GAME_START = 10, // 游戏开始
	GAME_CHECKOUT = 11, // 游戏结算
	ROOM_RECONNECT = 12, // 房间断线重连

	GEN_FRUIT = 13, // 产生了一个水果
	SLICE_FRUIT = 14, // 切水果的判定
	PLAYER_SEND_READY = 15, // 玩家准备

	AFTER_CHECKOUT = 16, // 结算时间结束
	PLAYER_SEND_PROP = 17, // 玩家发送道具
};

// 好友对战

#endif

