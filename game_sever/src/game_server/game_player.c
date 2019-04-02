#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "game_player.h"
#include "../netbus/netbus.h"
#include "../game_command.h"
#include "../game_stype.h"

#include "../database/center_database.h"
#include "../database/game_database.h"

int
player_load_uinfo(struct game_player* player) {
	struct user_info uinfo;
	if (get_uinfo_inredis(player->uid, &uinfo) < 0) {
		return -1;
	}

	strcpy(player->unick, uinfo.unick);
	player->usex = uinfo.usex;
	player->uface = uinfo.uface;


	return 0;
}

int
player_load_ugame_info(struct game_player* player) {
	struct ugame_common_info ugame_info;
	get_ugame_common_info_inredis(player->uid, &ugame_info);

	player->uchip = ugame_info.uchip;
	player->uexp = ugame_info.uexp;
	player->uvip = ugame_info.uvip;


	return 0;
}

int
player_load_uscore_info(struct game_player* player) {
	return 0;
}

// json array 对象，来生成我们的房间信息
// [zoneid, roomid, sv_seatid, ...]
json_t*
get_player_room_info(struct game_player* player) {
	json_t* json_array = json_new_array();
	json_array_push_number(json_array, player->zid);
	json_array_push_number(json_array, player->roomid);
	json_array_push_number(json_array, player->sv_seatid);


	return json_array;

}


// json array 对象，来生成我们的游戏信息
// [uchip, uvip, uexp, win_round, lost_round, ..]
json_t*
get_player_game_info(struct game_player* player) {
	json_t* json_array = json_new_array();
	json_array_push_number(json_array, player->uchip);
	json_array_push_number(json_array, player->uvip);
	json_array_push_number(json_array, player->uexp);

	json_array_push_number(json_array, player->win_round);
	json_array_push_number(json_array, player->lost_round);

	return json_array;
}

// json array 对象，来生成我们的玩家信息
// [unick, uface, usex]
json_t*
get_player_user_info(struct game_player* player) {
	json_t* json_array = json_new_array();
	json_array_push_string(json_array, player->unick);
	json_array_push_number(json_array, player->uface);
	json_array_push_number(json_array, player->usex);
	return json_array;
}

void 
player_init(struct game_player* player, unsigned int uid, int stype) {
	memset(player, 0, sizeof(struct game_player));
	player->uid = uid;
	player->stype = stype;
	player->status = PLAYER_INVIEW;
}

void
player_on_game_start(struct game_player* player) {
	player->status = PLAYER_INGAME;
}

static
void send_win_event_to_task_server(struct game_player* player) {
	json_t* cmd = json_new_server_return_cmd(STYPE_SYSTEM, TASK_WIN_EVENT, player->uid, 0);
	session_send_json_cmd_to_service(player->s, STYPE_SYSTEM, cmd);
}

void
player_on_game_checkout(struct game_player* player, int winner_seat) {
	player->status = PLAYER_INCHECKOUT;

	if (winner_seat == -1) { // 平局

	}
	else if (winner_seat == player->sv_seatid){ // 赢了
		send_win_event_to_task_server(player);
	}
	else  { // 输了

	}
}

void
player_on_after_checkout(struct game_player* player) {
	player->status = PLAYER_INVIEW;
}
