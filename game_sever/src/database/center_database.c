#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <hiredis.h>

#ifdef WIN32
#define NO_QFORKIMPL //这一行必须加才能正常使用
#include <Win32_Interop/win32fixes.h>
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")

#include <winsock.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")
#pragma comment(lib, "libmysql.lib")
#endif

#include <mysql.h>

#include "../netbus/netbus.h"
#include "../utils/log.h"
#include "center_database.h"
#include "center_db_config.h"

static void* mysql_center = NULL;
static redisContext* redis_center = NULL;

static int
select_redis_dbid(int dbid) {
	int ret = -1;
	if (dbid != 0) {
		redisReply*reply = redisCommand(redis_center, "select %d", dbid);
		if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK")) {
			LOGINFO("redis select %d database success!!\n", dbid);
			ret = 0;
		}
		else {
			if (reply->str) {
				LOGERROR("redis select %d database %s error !!!!\n", dbid, reply->str);
			}
		}
	}
	return ret;
}

void 
connect_to_center() {
	// 连接到中心数据库
	if (mysql_center == NULL) {
		mysql_center = mysql_init(NULL);
		if (!mysql_real_connect(mysql_center,
			CENTER_DB_CONF.mysql_ip, CENTER_DB_CONF.mysql_name,
			CENTER_DB_CONF.mysql_pwd, CENTER_DB_CONF.database_name,
			CENTER_DB_CONF.mysql_port, NULL, 0)) {
			LOGERROR("connect error!!! \n %s\n", mysql_error(mysql_center));
			mysql_close(mysql_center);
			mysql_center = NULL;

			// 启动一个定时器
			netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_center, NULL, 3.0f);
			// end 
		}
		else {
			LOGINFO("mysql connected success !!!!!");
			mysql_set_character_set(mysql_center, "utf8");
		}
	}
	// end 

	// 连接到redis
	if (redis_center == NULL) {
		struct timeval tv = { 1, 500000 };
		redis_center = redisConnectWithTimeout(CENTER_DB_CONF.redis_ip, CENTER_DB_CONF.redis_port, tv);
		if (redis_center->err != 0) { // 
			LOGERROR("Connection error: %s\n", redis_center->errstr);
			redisFree(redis_center);
			redis_center = NULL;
			// 连接中心redis失败,启动重连
			if (mysql_center != NULL) {
				// 启动一个定时器
				netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_center, NULL, 3.0f);
			}
			// end 
		}
		else {
			if (CENTER_DB_CONF.redis_pwd != NULL) { // 有密码
				redisReply*reply = redisCommand(redis_center, "auth %s", CENTER_DB_CONF.redis_pwd);
				if (reply) {
					if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) { // 验证通过
						LOGINFO("Connect center redis sucess!");
						if (CENTER_DB_CONF.redis_dbid != 0) {
							if (select_redis_dbid(CENTER_DB_CONF.redis_dbid) != 0) {
								redisFree(redis_center);
								redis_center = NULL;
								if (mysql_center != NULL) {
									netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_center, NULL, 3.0f);
								}
							}
						}
					}
					else {
						LOGERROR("Auth Error !!! %s\n", reply->str);
						redisFree(redis_center);
						redis_center = NULL;
						if (mysql_center != NULL) {
							netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_center, NULL, 3.0f);
						}
					}
					freeReplyObject(reply);
				}
			}
			else {
				LOGINFO("Connect center redis sucess!");
				if (CENTER_DB_CONF.redis_dbid != 0) {
					if (select_redis_dbid(CENTER_DB_CONF.redis_dbid) != 0) {
						redisFree(redis_center);
						redis_center = NULL;
						if (mysql_center != NULL) {
							netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_center, NULL, 3.0f);
						}
					}
				}
			}
		}
	}
	// end 
}


static char sql_cmd[2048];

int
get_uinfo_by_uid(unsigned int uid, struct user_info* uinfo) {
	if (mysql_center == NULL) {
		return -1;
	}
	char* sql_str = "select unick, phone_number, is_guest, uface, usex \
from uinfo \
where id = %d limit 1";
	sprintf(sql_cmd, sql_str, uid);

	LOGINFO("%s", sql_cmd);
	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_center);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (uinfo) {
			uinfo->uid = uid;
			strcpy(uinfo->unick, row[0]);
			strcpy(uinfo->phone_number, row[1]);
			uinfo->is_guest = atoi(row[2]);
			uinfo->uface = atoi(row[3]);
			uinfo->usex = atoi(row[4]);
		}
		return 0;
	}
	return -1;
}

int
get_uinfo_by_uname_upwd(char* uname, char* upwd, struct user_info* uinfo) {
	if (mysql_center == NULL) {
		return -1;
	}
	char* sql_str = "select id, unick, phone_number, is_guest, uface, usex \
from uinfo \
where uname = \"%s\" and upwd = \"%s\" and is_guest = 0 limit 1";
	sprintf(sql_cmd, sql_str, uname, upwd);
	LOGINFO("%s", sql_cmd);

	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_center);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (uinfo) {
			uinfo->uid = atoi(row[0]);
			strcpy(uinfo->unick, row[1]);
			strcpy(uinfo->phone_number, row[2]);
			uinfo->is_guest = atoi(row[3]);
			uinfo->uface = atoi(row[4]);
			uinfo->usex = atoi(row[5]);
		}
		return 0;
	}
	return -1;
}

int
get_uinfo_by_ukey(char* ukey, struct user_info* uinfo) {
        LOGINFO("mysql %s", mysql_center);
        if (mysql_center == NULL) {
		return -1;
	}
	char* sql_str = "select id, unick, phone_number, is_guest, uface, usex \
					from uinfo \
					where ukey = \"%s\" limit 1";
	sprintf(sql_cmd, sql_str, ukey);

	LOGINFO("%s", sql_cmd);
	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_center);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (uinfo) {
			uinfo->uid = atoi(row[0]);
			strcpy(uinfo->unick, row[1]);
			strcpy(uinfo->phone_number, row[2]);
			uinfo->is_guest = atoi(row[3]);
			uinfo->uface = atoi(row[4]);
			uinfo->usex = atoi(row[5]);
		}
		return 0;
	}
	return -1;
}

int
insert_guest_with_ukey(char* ukey, char* unick,
                       int uface, int usex) {
	if (mysql_center == NULL) {
		return -1;
	}
	char* fmt_sql = "insert into uinfo(`ukey`, `unick`, \
`uface`, `usex`)values(\"%s\", \"%s\", %d, %d)";
	sprintf(sql_cmd, fmt_sql, ukey, unick, uface, usex);

	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}

	return 0;
}

int
update_user_profile(unsigned int uid, 
                    char* unick, int uface, int usex) {
	if (mysql_center == NULL) {
		return -1;
	}

	char* fmt_sql = "update uinfo set unick = \"%s\", uface = %d, usex = %d where id = %d";
	sprintf(sql_cmd, fmt_sql, unick, uface, usex, uid);

	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}
	return 0; 
}

static void
update_user_upwd_with_value(unsigned int uid, char* new_upwd_md5) {
	char* fmt_sql = "update uinfo set upwd = \"%s\" where id = %d";
	sprintf(sql_cmd, fmt_sql, new_upwd_md5, uid);

	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return;
	}
}

int
update_user_upwd(unsigned int uid, char* old_upwd_md5, char* new_upwd_md5) {
	if (mysql_center == NULL) {
		return -1;
	}
	// 检查旧得密码
	char* fmt_sql = "select upwd from uinfo where id = %d limit 1";
	sprintf(sql_cmd, fmt_sql, uid);
	LOGINFO("%s", sql_cmd);
	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_center);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		char* old_pwd = row[0];
		if (strcmp(old_pwd, old_upwd_md5) != 0) { // 旧的密码不对
			return -1;
		}

		update_user_upwd_with_value(uid, new_upwd_md5);
		return 0;
	}
	return -1;
	// end 
}

int
is_uname_exist(unsigned int uid, char* uname) {
	if (mysql_center == NULL) {
		return -1;
	}

	char* fmt_sql = "select id from uinfo where uname = \"%s\" and is_guest = 0";
	sprintf(sql_cmd, fmt_sql, uname);

	if (mysql_query(mysql_center, sql_cmd)) {
		printf("select error %s\n", mysql_error(mysql_center));
		return -1;
	}
	else { 
		MYSQL_RES* res = mysql_store_result(mysql_center);
		MYSQL_ROW row;
		while (row = mysql_fetch_row(res)) { // 取完这行，再取下一行;
			unsigned int id = atoi(row[0]);
			if (uid != id) {
				return -1;
			}
		}
	}

	return 0;
}
//
int
upgrade_account(unsigned int uid, char* uname, char* upwd_md5) {
	if (mysql_center == NULL) {
		return -1;
	}

	char* fmt_sql = "update uinfo set uname = \"%s\", upwd = \"%s\", is_guest = 0 where id = %d";
	sprintf(sql_cmd, fmt_sql, uname, upwd_md5, uid);

	if (mysql_query(mysql_center, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_center));
		return -1;
	}
	return 0;
}

// redis
void 
set_uinfo_inredis(struct user_info* uinfo) {
	if (redis_center == NULL) {
		return;
	}

	char* fmt_redis_cmd = "HMSET uinfo_%d uid %d unick %s phone_number %s uface %d usex %d is_guest %d";
	redisReply*reply = redisCommand(redis_center, fmt_redis_cmd, uinfo->uid, uinfo->uid, uinfo->unick, uinfo->phone_number, uinfo->uface, uinfo->usex, uinfo->is_guest);
	if (reply) {
		if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) {
			LOGINFO("set uinfo in redis %d, %s success\n", uinfo->uid, uinfo->unick);
		}
		else {
			LOGERROR("set uinfo in redis error %s\n", reply->str);
		}
		freeReplyObject(reply);
	}
}

int 
get_uinfo_inredis(unsigned uid, struct user_info* uinfo) {
	if (uinfo == NULL) {
		return -1;
	}
	int ret = -1;
	char* fmt_redis_cmd = "hgetall uinfo_%d";
	redisReply*reply = redisCommand(redis_center, fmt_redis_cmd, uid);
	if (reply) {
		if (reply->type == REDIS_REPLY_ARRAY) { // 查询结果是一个数组
			if (reply->elements == 12) {
				uinfo->uid = uid;
				strcpy(uinfo->unick, reply->element[3]->str);
				strcpy(uinfo->phone_number, reply->element[5]->str);
				uinfo->uface = atoi(reply->element[7]->str);
				uinfo->usex = atoi(reply->element[9]->str);
				uinfo->is_guest = atoi(reply->element[11]->str);
				ret = 0;
			}
			else {
				LOGERROR("get_uinfo_inredis error:%d, eleme count %d\n", uid, reply->elements);
			}
		}
		else {
			if (reply->str) {
				LOGERROR("get_uinfo_inredis error:%d, %s\n", uid, reply->str);
			}
		}
		freeReplyObject(reply);
	}
	return ret;
}

