#ifndef __GAME_ROOM_H__
#define __GAME_ROOM_H__

#include "../netbus/netbus.h"
#include "../utils/cache_allocer.h"

#include "internet_server.h"

struct internet_server;

enum {
	ROOM_REDAY = 0, // 房间已经准备好了
	ROOM_INGAME = 1, // 房间已经开始游戏了
	ROOM_INCHECKOUT = 2, // 房间在结算中
};
// 创建房间
struct game_room;

struct cache_alloc* 
create_room_allocer(int room_num);

struct cache_alloc*
create_fruit_allocer(int fruit_num);

struct game_room*
create_game_room(struct internet_server* s, int zid);

// 销毁房间
void
destroy_game_room(struct game_room* room);

int 
is_room_can_enter(struct game_room* room, int* player_num);

// 玩家进入房间
void
enter_game_room(struct game_room* room, struct game_player* player);

// 玩家尝试退出room
// 如果退出成功，return 1,否则return 0
int
player_try_exit_room(struct game_room* room, struct game_player* player);

// 玩家强退出
int
player_force_exit_room(struct game_room* room, struct game_player* player);

// 玩家断线重连
int
player_reconnect_room(struct game_room* room, struct game_player* player);

// 获取其它玩家在房间的信息
json_t*
get_other_players_info(struct game_room* room, struct game_player* yourself);
//

// 获取房间的状态信息
json_t*
get_room_status_info(struct game_room* room);

// end
int
player_slice_fruit(struct game_room* room, struct game_player* player, int fruit_id, int degree);

int
player_set_ready_in_room(struct game_room* room, struct game_player* player);

int
player_send_prop_in_room(struct game_room* room, struct game_player* player, int to_seatid, int prop_id);
#endif

