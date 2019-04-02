#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/log.h"

#include "../game_stype.h"
#include "gw_config.h"
#include "../netbus/netbus.h"
#include "server_session.h"
#include "from_client.h"
#include "server_return.h"
#include "session_key_man.h"
#include "server_post.h"

#ifdef GAME_DEVLOP
#include "../bycw_center_server/center_services.h"
#include "../bycw_common_server/common_services.h"
#include "../bycw_game_server/game_internet_service.h"
#include "../database/center_database.h"
#include "../database/game_database.h"
#endif

int main(int argc, char** argv) {
	init_log();
	init_session_key_man();
	init_server_netbus(WEB_SOCKET_IO, JSON_PROTOCAL);
	// init_server_netbus(WEB_SOCKET_IO, BIN_PROTOCAL);
	// init_server_netbus(TCP_SOCKET_IO, BIN_PROTOCAL);
	// init_server_netbus(TCP_SOCKET_IO, JSON_PROTOCAL);
	// 注册服务的模块;
	
#ifdef GAME_DEVLOP // 方便调试，单进程开发所有的服务
	connect_to_center();
	connect_to_game();
	register_service(STYPE_SERVER_POST, &SERVER_POST_SERVICE);
	register_service(STYPE_CENTER, &CENTER_SERVICES);
	register_service(STYPE_SYSTEM, &COMMON_SERVICES);
	register_service(STYPE_GAME_INTERNET, &GAME_INTERNET_SERVICES);
#else
	register_service(STYPE_SERVER_POST, &SERVER_POST_SERVICE);
	// for (int i = 0; i < GW_CONFIG.num_server_moudle; i++) {
	for (int i = 0; i < 3; i++) { // 
		register_from_client_module(GW_CONFIG.module_set[i].stype);
		register_server_return_module(GW_CONFIG.module_set[i].stype);
	}
	init_server_session_man();
#endif
	// end
	
	LOGINFO("start server at %s:%d\n", GW_CONFIG.ip, GW_CONFIG.port);

	start_server(GW_CONFIG.ip, GW_CONFIG.port);
	exit_server_netbus();
	exit_session_key_man();

	return 0;
}
