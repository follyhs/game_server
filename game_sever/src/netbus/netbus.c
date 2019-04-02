#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/timer.h"
#include "../utils/timer_list.h"

#include "socket/session.h"
#include "netbus.h"

#include "../game_command.h"
#include "../game_stype.h"

#ifdef GAME_DEVLOP
#include "../bycw_gateway_server/session_key_man.h"
#endif


struct timer_list* GATEWAY_TIMER_LIST = NULL;

#define MAX_SERVICES 512

struct {
	struct service_module* services[MAX_SERVICES];
}netbus;


void
init_server_netbus(int socket_type, int protocal_type) {
	memset(&netbus, 0, sizeof(netbus));
	GATEWAY_TIMER_LIST = create_timer_list();
	init_session_manager(socket_type, protocal_type);
}

void
exit_server_netbus() {
	exit_session_manager();
	destroy_timer_list(GATEWAY_TIMER_LIST);
}

void
register_service(int stype, struct service_module* module) {
	if (stype <= 0 || stype >= MAX_SERVICES) {
		// 打印error
		return;
	}

	netbus.services[stype] = module;
	if (module->init_service_module) {
		module->init_service_module(module);
	}
}


void
on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	unsigned char save = data[len];
	data[len] = 0;
	json_t* root = NULL;
	int ret = json_parse_document(&root, data);
	if (ret != JSON_OK || root == NULL) { // 不是一个正常的json包;
		data[len] = save;
		return;
	}

	// 获取这个命令服务类型，假定0(key)为服务器类型；
	json_t* server_type = json_find_first_label(root, "0");
	server_type = server_type->child;
	if (server_type == NULL || server_type->type != JSON_NUMBER) {
		goto ended;
	}
	int stype = atoi(server_type->text); // 获取了我们的服务号。
	if (stype < 0 || stype >= MAX_SERVICES) {
		goto ended;
	}

	if (netbus.services[stype] && netbus.services[stype]->on_json_protocal_recv) {
#ifdef GAME_DEVLOP
		unsigned int key;
		if (s->is_server_session == 0) {
			json_object_push_number(root, "uid", s->uid);
			key = get_key_from_session_map();
			save_session_with_key(key, s);
			json_object_push_number(root, "s_key", key);
		}
#endif
		int ret = netbus.services[stype]->on_json_protocal_recv(netbus.services[stype]->module_data, 
			                                          s, root, data, len);

#ifdef GAME_DEVLOP
		if (s->is_server_session == 0) {
			clear_session_with_key(key);
		}
#endif
		if (ret < 0) {
			close_session(s);
		}
	} 
	// end 
ended:
	data[len] = save;
	json_free_value(&root);
}

// 假设前面4个字节为服务类型
void 
on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	int stype = ((data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	if (netbus.services[stype] && netbus.services[stype]->on_bin_protocal_recv) {
		int ret = netbus.services[stype]->on_bin_protocal_recv(netbus.services[stype]->module_data,
			s, data, len);
		if (ret < 0) {
			close_session(s);
		}
	}
}

void
on_connect_lost_entry(struct session* s) {
	for (int i = 0; i < MAX_SERVICES; i++) {
		if (netbus.services[i] && netbus.services[i]->on_connect_lost) {
			netbus.services[i]->on_connect_lost(netbus.services[i]->module_data, s);
		}
	}

#ifdef GAME_DEVLOP
	clear_session_with_uid(s->uid);
#endif
}

void
session_send_json_cmd_to_service(struct session* s, int to_stype, json_t* cmd) {
	if (to_stype > MAX_SERVICES || to_stype < 0) {
		return;
	}

#ifdef GAME_DEVLOP
	if (netbus.services[to_stype]) {
		netbus.services[to_stype]->on_json_protocal_recv(netbus.services[to_stype]->module_data, s, cmd, NULL, 0);
		json_free_value(&cmd);
	}
#else
	json_t* json = json_new_command(STYPE_SERVER_POST, to_stype);
	json_insert_pair_into_object(json, "2", cmd);
	session_send_json(s, json);
	json_free_value(&json);
#endif
}

unsigned int
netbus_add_timer(void(*on_timer)(void* udata),
                  void* udata, float after_sec) {
	return add_timer(GATEWAY_TIMER_LIST, on_timer, udata, after_sec);
}

void
netbus_cancel_timer(unsigned int timeid) {
	cancel_timer(GATEWAY_TIMER_LIST, timeid);
}

unsigned int
netbus_schedule(void(*on_timer)(void* udata),
void* udata, float after_sec)
{
	return schedule_timer(GATEWAY_TIMER_LIST, on_timer, udata, after_sec);
}

