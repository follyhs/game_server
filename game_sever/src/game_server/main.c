#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/log.h"
#include "gserver_config.h"
#include "../netbus/netbus.h"
#include "../game_stype.h"

#include "../database/center_database.h"
#include "../database/game_database.h"
#include "game_internet_service.h"


int main(int argc, char** argv) {
	init_log();
	
	// init_server_netbus(WEB_SOCKET_IO, JSON_PROTOCAL);
	// init_server_netbus(WEB_SOCKET_IO, BIN_PROTOCAL);
	// init_server_netbus(TCP_SOCKET_IO, BIN_PROTOCAL);
	init_server_netbus(TCP_SOCKET_IO, JSON_PROTOCAL);
	connect_to_center();
	connect_to_game();
	// 注册服务的模块;
	register_service(STYPE_GAME_INTERNET, &GAME_INTERNET_SERVICES);
	// end
	start_server(GSERVER_CONF.ip, GSERVER_CONF.port);
	exit_server_netbus();

	return 0;
}
