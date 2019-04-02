#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../netbus/netbus.h"

#include "game_db_config.h"

struct game_db_config GAME_DB_CONF = {
	// MYSQL
	"localhost",
	"root",
	3306,
	"123456",
	"bycw_game",
	// end

	// redis
	"localhost",
	6379,
	NULL, // 没有密码就为NULL，"123456"
	0,   // 0号数据仓库来存放我们的中心服务器的数据的redis数据库
	// end
};
