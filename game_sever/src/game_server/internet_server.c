#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../netbus/netbus.h"
#include "../game_stype.h"
#include "../game_command.h"
#include "../game_result.h"

#include "internet_server.h"
#include "models/model_internet_server.h"
#include "gserver_config.h"
#include "game_player.h"
#include "game_room.h"

#define my_malloc malloc
#define my_free free

static int 
find_one_player_room_cond(void* room, void* udata) {
	int player_num;
	if (!is_room_can_enter(room, &player_num)) {
		return 0;
	}

	if (player_num == 1) {
		return 1;
	}
	return 0;
}

static int
find_room_cond(void* room, void* udata) {
	int player_num;
	if (!is_room_can_enter(room, &player_num)) {
		return 0;
	}

	return 1;
}

static void 
alloc_room_in_zone(struct internet_server* server, int zid) {
	struct game_player* walk = server->wait_list_set[zid - 1];
	server->wait_list_set[zid - 1] = NULL;

	while (walk) {
		struct game_player* player = walk;
		walk = walk->wait_next;
		player->wait_next = NULL;

		// 找一个合适的room
		struct game_room* room;
		room = for_each_find(server->room_set, find_one_player_room_cond, NULL);
		if (room == NULL) {
			room = for_each_find(server->room_set, find_room_cond, NULL);
		}
		
		if (room == NULL) {
			room = create_game_room(server, zid);
		}
		if (room == NULL) {
			LOGERROR("server cannot alloc the room for game!!!");
			return;
		}
		// end 

		// 找到了合适的房间
		enter_game_room(room, player);
		// end 
	}

	
}

static void 
alloc_room(struct internet_server* server) {
	for (int i = 0; i < ZONE_NUM; i++) {
		alloc_room_in_zone(server, i + 1);
	}
}

struct internet_server*
create_ineternet_server() {
	struct internet_server* server = my_malloc(sizeof(struct internet_server));
	memset(server, 0, sizeof(struct internet_server));

	server->stype = STYPE_GAME_INTERNET;
	server->desic = "game internet service";
	server->online_num = 0;
	server->room_num = 0;

	// 创建一个uid-->player的hash map 映射表
	server->player_set = create_hash_int_map();
	// end 

	// 创建一个roomid --> game_room 对象的hashmap映射表
	server->room_set = create_hash_int_map();
	// end 

	// 创建玩家的内存分配池
	server->player_allocer = create_cache_alloc(GSERVER_CONF.max_cache_player, sizeof(struct game_player));
	// end 

	// 创建游戏的房间分配池
	server->room_allocer = create_room_allocer(GSERVER_CONF.max_cache_room);
	server->auto_inc_roomid = 1;
	// end 

	// 创建我们的游戏水果的分配器
	server->fruit_allocer = create_fruit_allocer(GSERVER_CONF.max_cache_fruit);
	// end 

	// 启动定时器来配卓
	netbus_schedule(alloc_room, server, 0.5f);
	// end 
	return server;
}

/*
0: 服务号
1: 命令号
返回
状态码 --OK...
*/
void
enter_internet_server(void* module_data, json_t* json,
                      struct session* s,
					  unsigned int uid, unsigned int s_key) {
	// step1 检查命令的格式长度
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_SERVER, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end 

	int ret = model_enter_internet_server((struct internet_server*) module_data, uid, s);
	if (ret != MODEL_INTERNET_SERVER_SUCCESS) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_SERVER, SYSTEM_ERROR, uid, s_key);
		return;
	}

	// 登陆到internet服务器 成功
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, ENTER_SERVER, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}

/*
0: 服务号
1: 命令号
返回:
0: 服务号
1: 命令号
2:status状态码 OK
*/
void
exit_internet_server(void* module_data, json_t* json,
                     struct session* s,
				     unsigned int uid, unsigned int s_key) {
	// 
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_SERVER, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end

	// 查找这个服务器里面有没有玩家
	int ret = model_exit_internet_server((struct internet_server*) module_data, uid, 1);
	if (ret == MODEL_INTERNET_SERVER_USER_IS_NOT_IN_SERVER) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_SERVER, USER_IS_NOT_IN_SEVER, uid, s_key);
		return;
	}
	if (ret == MODEL_INTERNET_SERVER_USER_IS_IN_GAME) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_SERVER, USER_IS_IN_GAME, uid, s_key);
		return;
	}
	// end 


	// 离开到internet服务器 成功
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, EXIT_SERVER, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}

/*
0: 服务号
1: 命令号
2: 区间号 [0(系统分配), 1, 2, 3]
返回:
0: 服务号
1: 命令号
status OK, 
zid
*/
void
enter_internet_server_zone(void* module_data, json_t* json,
                           struct session* s,
						unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 3 + 2) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end

	int zid = json_object_get_number(json, "2");
	if (zid < 0 || zid > 3) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, INVALID_PARAMS, uid, s_key);
		return;
	}

	if (zid == INVALIDI_ZONE) { // 自动分配
		zid = model_auto_get_zoneid((struct internet_server*) module_data, uid); 
		if (zid == INVALIDI_ZONE) {
			write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, INVALIDI_ZOOM_ID, uid, s_key);
			return;
		}
	}

	int ret = model_enter_internet_server_zone((struct internet_server*) module_data, uid, zid);
	if (ret == MODEL_INTERNET_SERVER_USER_RECONNECT) {
		model_player_reconnect_to_room((struct internet_server*) module_data, uid);
		// write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, USER_IS_IN_ZONE, uid, s_key);
		return;
	}

	if (ret == MODEL_INTERNET_SERVER_USER_IS_NOT_IN_SERVER) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, USER_IS_NOT_IN_SEVER, uid, s_key);
		return;
	}
	if (ret == MODEL_INTERNET_SERVER_USER_IS_IN_ZONE) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, USER_IS_IN_ZONE, uid, s_key);
		return;
	}
	if (ret == MODEL_INTERNET_SERVER_INVALID_ZONE) {
		write_error(s, STYPE_GAME_INTERNET, ENTER_ZONE, INVALIDI_ZOOM_ID, uid, s_key);
		return;
	}

	// 成功返回
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, ENTER_ZONE, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	json_object_push_number(ret_cmd, "3", zid);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}

/*
0: 服务号,
1: 命令
返回:
0: 服务号
1: 命令好
2: status OK, ...
*/
void
exit_internet_server_zone(void* module_data, json_t* json,
                          struct session* s,
                          unsigned int uid, unsigned int s_key) {
	int len = json_object_size(json);
	if (len != 3 + 2) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_ZONE, INVALID_PARAMS, uid, s_key);
		return;
	}

	int ret = model_exit_internet_server_zone((struct internet_server*) module_data, uid);
	if (ret == MODEL_INTERNET_SERVER_USER_IS_NOT_IN_SERVER) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_ZONE, USER_IS_NOT_IN_SEVER, uid, s_key);
		return;
	}
	if (ret == MODEL_INTERNET_SERVER_USER_IS_NOT_IN_ZONE) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_ZONE, USER_IS_NOT_IN_ZONE, uid, s_key);
		return;
	}
	if (ret == MODEL_INTERNET_SERVER_USER_IS_IN_GAME) {
		write_error(s, STYPE_GAME_INTERNET, EXIT_ZONE, USER_IS_IN_GAME, uid, s_key);
		return;
	}
	// 成功
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, EXIT_ZONE, uid, s_key);
	json_object_push_number(ret_cmd, "2", OK);
	session_send_json(s, ret_cmd);
	json_free_value(&ret_cmd);
	// end 
}

/*
玩家进入房间，服务器主动的通知客户端
0: 服务号;
1: 命令号;
2: roominfo [zoneid, roomid, sv_seat]
3: uinfo [unick, uface, usex]
4: gameinfo [round, start, ....]


*/
void
send_user_enter_room(struct game_room* room, struct game_player* player) {
	if (player->online == 0 || player->s == NULL) {
		return;
	}

	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, ENTER_ROOM, player->uid, 0);
	json_t* room_info = get_player_room_info(player);
	json_insert_pair_into_object(ret_cmd, "2", room_info);

	json_t* user_info = get_player_user_info(player);
	json_insert_pair_into_object(ret_cmd, "3", user_info);

	json_t* game_info = get_player_game_info(player);
	json_insert_pair_into_object(ret_cmd, "4", game_info);

	session_send_json(player->s, ret_cmd);
	json_free_value(&ret_cmd);
}

/*
玩家进入房间，服务器主动的通知客户端
0: 服务号;
1: 命令号;
2: roominfo [zoneid, roomid, sv_seat]
3: uinfo [unick, uface, usex]
4: gameinfo [round, start, ....]


*/
void
send_user_arrive_room(struct game_room* room, struct game_player* player, struct game_player* to_player) {
	if (to_player->online == 0 || to_player->s == NULL) {
		return;
	}

	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, USER_ARRIVED, to_player->uid, 0);
	json_t* room_info = get_player_room_info(player);
	json_insert_pair_into_object(ret_cmd, "2", room_info);

	json_t* user_info = get_player_user_info(player);
	json_insert_pair_into_object(ret_cmd, "3", user_info);

	json_t* game_info = get_player_game_info(player);
	json_insert_pair_into_object(ret_cmd, "4", game_info);

	session_send_json(to_player->s, ret_cmd);
	json_free_value(&ret_cmd);
}

/*
有玩家离开，服务器主动通知
0: 服务号
1: 命令号
2: 状态, 
3: 站起的座位号
*/
void
send_user_standup_room(struct game_room* room, struct game_player* to_player, int sv_seatid) {
	if (to_player->online == 0 || to_player->s == NULL) {
		return;
	}

	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, USER_STANDUP, to_player->uid, 0);
	json_object_push_number(ret_cmd, "2", OK);
	json_object_push_number(ret_cmd, "3", sv_seatid);
	session_send_json(to_player->s, ret_cmd);
	json_free_value(&ret_cmd);
}

/*
0: 服务号
1: 命令号
2:roominfo [zoneid, roomid, sv_seat]
3:uinfo [unick, uface, usex]
4: gameinfo [round, start, ....]
5: 对家[unick, uface, usex]
6: 房间的状态[room_status, 游戏剩下的时间, [[seatid, score], [seatid, score]]]

*/
void 
send_user_reconnect_room(struct game_room* room, struct game_player* to_player) {
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, ROOM_RECONNECT, to_player->uid, 0);
	json_t* room_info = get_player_room_info(to_player);
	json_insert_pair_into_object(ret_cmd, "2", room_info);

	json_t* user_info = get_player_user_info(to_player);
	json_insert_pair_into_object(ret_cmd, "3", user_info);

	json_t* game_info = get_player_game_info(to_player);
	json_insert_pair_into_object(ret_cmd, "4", game_info);

	json_t* other_player_info = get_other_players_info(room, to_player);
	json_insert_pair_into_object(ret_cmd, "5", other_player_info);

	json_t* room_status = get_room_status_info(room);
	json_insert_pair_into_object(ret_cmd, "6", room_status);

	session_send_json(to_player->s, ret_cmd);
	json_free_value(&ret_cmd);
}

/*
0: 服务号
1: 命令号
2: 水果的id
3: 切到水果的角度

返回: 房间广播结果
*/

void
slice_fruit_internet_server(void* module_data, json_t* json,
                            struct session* s,
							unsigned int uid, unsigned int s_key) {
	// 检查协议合法性
	int len = json_object_size(json);
	if (len != 4 + 2) {
		write_error(s, STYPE_GAME_INTERNET, SLICE_FRUIT, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end 

	int fruit_id = json_object_get_number(json, "2");
	int degree = json_object_get_number(json, "3");
	// 根据uid 获取 game_player
	int ret = model_slice_fruit((struct internet_server*)module_data, uid, fruit_id, degree);
	if (ret != MODEL_INTERNET_SERVER_SUCCESS) {
		write_error(s, STYPE_GAME_INTERNET, SLICE_FRUIT, INVALID_OPT, uid, s_key);
		return;
	}
}

/*
0: 服务号
1: 命令号
*/
void 
player_send_ready_in_room(void* module_data, json_t* json,
                          struct session* s,
						  unsigned int uid, unsigned int s_key) {
	// 检查协议合法性
	int len = json_object_size(json);
	if (len != 2 + 2) {
		write_error(s, STYPE_GAME_INTERNET, PLAYER_SEND_READY, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end 

	int ret = model_player_set_ready_in_room((struct internet_server*)module_data, uid);
	if (ret != MODEL_INTERNET_SERVER_SUCCESS) {
		write_error(s, STYPE_GAME_INTERNET, PLAYER_SEND_READY, INVALID_OPT, uid, s_key);
		return;
	}

	// Ok
	json_t* ret_cmd = json_new_server_return_cmd(STYPE_GAME_INTERNET, PLAYER_SEND_READY, uid, 0);
	json_object_push_number(ret_cmd, "2", OK);
	json_free_value(&ret_cmd);
	// end 
}

/*
0: 服务号
1: 命令号
2: to_seatid
3: prop_id
*/
void
send_prop_in_room(void* module_data, json_t* json,
                  struct session* s,
	              unsigned int uid, unsigned int s_key) {
	// 检查协议合法性
	int len = json_object_size(json);
	if (len != 4 + 2) {
		write_error(s, STYPE_GAME_INTERNET, PLAYER_SEND_PROP, INVALID_PARAMS, uid, s_key);
		return;
	}
	// end 

	int seatid = json_object_get_number(json, "2");
	int propid = json_object_get_number(json, "3");
	int ret = model_player_send_prop_in_room((struct internet_server*)module_data, uid, seatid, propid);
	if (ret != MODEL_INTERNET_SERVER_SUCCESS) {
		write_error(s, STYPE_GAME_INTERNET, PLAYER_SEND_PROP, INVALID_OPT, uid, s_key);
		return;
	}
}