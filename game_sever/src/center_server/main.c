#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/log.h"
#include "../netbus/netbus.h"
#include "../game_stype.h"
#include "cener_config.h"
#include "center_services.h"
//#include "../database/center_database.h"

int main(int argc, char** argv) {
	init_log();
	LOGINFO("center server starting !!!!!\n");
	// init_server_netbus(WEB_SOCKET_IO, JSON_PROTOCAL);
	// init_server_netbus(WEB_SOCKET_IO, BIN_PROTOCAL);
	// init_server_netbus(TCP_SOCKET_IO, BIN_PROTOCAL);
	init_server_netbus(TCP_SOCKET_IO, JSON_PROTOCAL);
	connect_to_center();
	// 注册服务的模块;
	register_service(STYPE_CENTER, &CENTER_SERVICES);
	// end
	start_server(CENTER_CONF.ip, CENTER_CONF.port);
	exit_server_netbus(); 

	return 0;
}
