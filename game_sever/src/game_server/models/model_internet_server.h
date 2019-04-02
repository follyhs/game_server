#ifndef __MODEL_INTERNET_SERVER_H__
#define __MODEL_INTERNET_SERVER_H__

#include "../internet_server.h"

enum {
	MODEL_INTERNET_SERVER_SUCCESS = 0, // 操作成功
	MODEL_INTERNET_SERVER_ERROR = -1, // 通用的error
	MODEL_INTERNET_SERVER_USER_IS_NOT_IN_SERVER = -2, // 玩家不在服务器上
	MODEL_INTERNET_SERVER_USER_IS_IN_ZONE = -3, // 用户已经在分区里面
	MODEL_INTERNET_SERVER_INVALID_ZONE = -4, // 
	MODEL_INTERNET_SERVER_USER_IS_NOT_IN_ZONE = -5, // 用户还没有被分区
	MODEL_INTERNET_SERVER_USER_IS_IN_GAME = -6, // 用户正在游戏中，无法退出
	MODEL_INTERNET_SERVER_USER_RECONNECT = -7, // 用户正在房间里面，准备断线从连
	MODEL_INTERNET_SERVER_USER_IS_NOT_IN_ROOM = -8, // 用户不在房间里面
	MODEL_INTERNET_SERVER_ROOM_IS_NOT_EXIST = -9, // 房间不存在
	MODEL_INTERNET_SERVER_ROOM_FUIT_IS_NOT_EXIST = -10, // 水果不存在
	MODEL_INTERNET_SERVER_FRUIT_IS_SLICED = -11,  // 水果已经被切掉
	MODEL_INTERNET_SERVER_INVALID_OPT = -12, // 非法的操作
	MODEL_INTERNET_SERVER_INVALID_PARAMS = -13, // 参数错误
};

int
model_enter_internet_server(struct internet_server* server, unsigned int uid, struct session* s);

int
model_exit_internet_server(struct internet_server* server, unsigned int uid, int is_online);

int
model_enter_internet_server_zone(struct internet_server* server, unsigned int uid, int zid);

int
model_auto_get_zoneid(struct internet_server* server, unsigned int uid);

int
model_exit_internet_server_zone(struct internet_server* server, unsigned int uid);

int
model_player_reconnect_to_room(struct internet_server* server, unsigned int uid);

int
model_slice_fruit(struct internet_server* server, unsigned int uid, int fruit_id, int degree);

int
model_player_set_ready_in_room(struct internet_server* server, unsigned int uid);

int
model_player_send_prop_in_room(struct internet_server* server, unsigned int uid, int to_seatid, int prop_id);
#endif

