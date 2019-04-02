#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/log.h"
#include "../game_stype.h"
#include "common_config.h"
#include "../netbus/netbus.h"
#include "common_services.h"
#include "../database/center_database.h"
#include "../database/game_database.h"

int main(int argc, char** argv) {
	init_log();
	LOGINFO("system common server starting !!!!!\n");
	// init_server_netbus(WEB_SOCKET_IO, JSON_PROTOCAL);
	// init_server_netbus(WEB_SOCKET_IO, BIN_PROTOCAL);
	// init_server_netbus(TCP_SOCKET_IO, BIN_PROTOCAL);
	init_server_netbus(TCP_SOCKET_IO, JSON_PROTOCAL);
	connect_to_center();
	connect_to_game();
	// 注册服务的模块;
	register_service(STYPE_SYSTEM, &COMMON_SERVICES);
	// end

	start_server(COMMON_CONF.ip, COMMON_CONF.port);
	exit_server_netbus();

	return 0;
}
