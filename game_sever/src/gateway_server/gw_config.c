#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../game_stype.h"
#include "gw_config.h"

struct server_module_config modules[] = {
	{
		STYPE_CENTER,
		"127.0.0.1",
		6080,
		"center server",
	},
	{
		STYPE_SYSTEM,
		"127.0.0.1",
		6081,
		"system server",
	},
	{
		STYPE_GAME_INTERNET,
		"127.0.0.1",
		6082,
		"game1 server",
	},

	{
		STYPE_GAME_FRIEND,
		"127.0.0.1",
		6083,
		"game1 server",
	},
};

struct gw_config GW_CONFIG = {
	"0.0.0.0",
	6090,
	sizeof(modules) / sizeof(struct server_module_config),
	modules,
};
