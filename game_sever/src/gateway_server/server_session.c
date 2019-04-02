#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../netbus/netbus.h"
#include "gw_config.h"
#include "server_session.h"

#define MAX_SERVER 16
struct {
	int need_conect; // 是否需要去检查
	struct session* server_session[MAX_SERVER];
}SESSION_MAN;

struct session*
get_server_session(int stype) {
	return SESSION_MAN.server_session[stype];
}

void
init_server_session_man() {
	memset(&SESSION_MAN, 0, sizeof(SESSION_MAN));
	SESSION_MAN.need_conect = 1;

	// 每隔3秒去检测一次，看有没有断掉的
	netbus_schedule(keep_servers_online, NULL, 4);
	// end 
}

void
keep_servers_online() {
	if (SESSION_MAN.need_conect == 0) {
		return;
	}
	SESSION_MAN.need_conect = 0;
	int i = 0;
	// for (i = 0; i < GW_CONFIG.num_server_moudle; i++) {
	for (i = 0; i < 3; i ++) {
		int stype = GW_CONFIG.module_set[i].stype;
		if (SESSION_MAN.server_session[stype] == NULL) {
			SESSION_MAN.server_session[stype] = netbus_connect(GW_CONFIG.module_set[i].ip, GW_CONFIG.module_set[i].port);
			if (SESSION_MAN.server_session[stype] == NULL) {
				SESSION_MAN.need_conect = 1;
				LOGINFO("connected to %s error!!!!!", GW_CONFIG.module_set[i].desic);
			}
			else {
				LOGINFO("connected to %s success!!!!!", GW_CONFIG.module_set[i].desic);
			}
		}
	}
	
}

void
disconnect_server(int stype) {
	SESSION_MAN.server_session[stype] = NULL;
	SESSION_MAN.need_conect = 1;
}
