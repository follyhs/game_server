#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gserver_config.h"

struct gserver_config GSERVER_CONF = {
	"0.0.0.0",  
	6082,

	10000, // player 缓冲池大小
	10000, // room 缓存大小
	100000, // 水果缓存的大小

	{60, 60, 60, 0, 0, 0, 0, 0, 0, 0},  // 每个zid的游戏时间

	5, // 游戏结算时间
};

