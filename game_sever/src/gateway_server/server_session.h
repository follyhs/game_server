#ifndef __SERVER_SESSION_H__
#define __SERVER_SESSION_H__

#include "../netbus/netbus.h"

void
init_server_session_man();

void 
keep_servers_online();

struct session*
get_server_session(int stype);

void
disconnect_server(int stype);

#endif

