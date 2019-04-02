#ifndef __CENTER_CONFIG_H__
#define __CENTER_CONFIG_H__

struct common_config {
	char* ip;
	int port;

	// 每日登录奖励配置
	int login_bonues_straight[7]; // 奖励的数目
	int max_straight_days; // 最多允许连续登录的天数
	// end

	// 第一次注册游戏账号送的金币
	int uchip;
	int uexp;
	int uvip;
	// end
};

extern struct common_config COMMON_CONF;
#endif

