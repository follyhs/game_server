#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

#include "../netbus/netbus.h"

enum {
	PLAYER_INVIEW = 0, // 玩家旁观状态，
	PLAYER_READY = 1, // 玩家准备好了；
	PLAYER_INGAME = 2, // 玩家正在游戏中;
	PLAYER_INCHECKOUT = 3, // 玩家正在结算
};

struct game_player {
	unsigned int uid; // 玩家的uid
	// 当前这个玩家所在服务器的服务号
	int stype;
	int online;
	struct session* s; // 保存这个玩家，发送数据的session
	// end 

	// 玩家的用户信息
	char unick[32];
	int usex;
	int uface;
	// end 

	// 玩家的游戏信息
	int uchip;
	int uvip;
	int uexp;
	// end 

	// 玩家的战绩信息
	int win_round;
	int lost_round;
	// end 

	// 玩家游戏中的服务分区信息
	int zid;
	int roomid; // 说明它还在等待列表里面，如果房间id为0的话
	int sv_seatid; // 玩家在游戏中的座位号
	int status; // 记录玩家的状态

	struct game_player* wait_next; // 等待列表里面的下一个节点
	// end 

};

void
player_init();

int
player_load_uinfo(struct game_player* player);

int
player_load_ugame_info(struct game_player* player);

int
player_load_uscore_info(struct game_player* player);

// json array 对象，来生成我们的房间信息
json_t* 
get_player_room_info(struct game_player* player);

// json array 对象，来生成我们的游戏信息
json_t* 
get_player_game_info(struct game_player* player);

// json array 对象，来生成我们的玩家信息
json_t*
get_player_user_info(struct game_player* player);

void
player_on_game_start(struct game_player* player);

void
player_on_game_checkout(struct game_player* player, int winner_seat);

void
player_on_after_checkout(struct game_player* player);

#endif

