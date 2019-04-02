#ifndef __CENTER_DB_CONFIG_H__
#define __CENTER_DB_CONFIG_H__


struct game_db_config {
	// 数据库
	char* mysql_ip;
	char* mysql_name;
	int mysql_port;
	char* mysql_pwd;
	char* database_name;
	// end 

	// redis
	char* redis_ip; // redis服务器的IP地址
	int redis_port; // redis服务器的连接端口
	char* redis_pwd; // redis服务器的密码
	int redis_dbid; // center所在的redis数据库的id号
	// end
};

extern struct game_db_config GAME_DB_CONF;
#endif

