#ifndef __CENTER_CONFIG_H__
#define __CENTER_CONFIG_H__

struct gserver_config {
	char* ip;
	int port;

	// player 缓冲池大小
	int max_cache_player;
	// room 缓存大小
	int max_cache_room;
	// 水果缓存的大小
	int max_cache_fruit;

	int zid_time_set[10]; // 每个zid的游戏时间
	int checkout_time; // 游戏结算时间
};

extern struct gserver_config GSERVER_CONF;
#endif

