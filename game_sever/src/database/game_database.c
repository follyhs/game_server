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
#include "../utils/timestamp.h"
#include "game_database.h"
#include "game_db_config.h"

static void* mysql_game = NULL;
static redisContext* redis_game = NULL;
static char sql_cmd[2048];

static int
select_redis_dbid(int dbid) {
	int ret = -1;
	if (dbid != 0) {
		redisReply*reply = redisCommand(redis_game, "select %d", dbid);
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

static int
mask_compare(const void* lhs, const void* rhs) {
	struct game_mask* mask_lhs = (struct game_mask*)lhs;
	struct game_mask* mask_rhs = (struct game_mask*)rhs;

	if (mask_lhs->level < mask_rhs->level) {
		return -1;
	}
	else {
		return 1;
	}
}

struct game_mask_config MASK_CONFIG;

static void
load_game_mask_config(struct game_mask_config* mask_config) {
	if (mysql_game == NULL) {
		return;
	}

	for (int i = WIND_ROUND_MASK; i <= MASK_NUM; i++) {
		vector_define(&mask_config->mask_config_set[i - 1], sizeof(struct game_mask));
		// 读取 win_round任务配置
		char* fmt_sql = "select * from game_mask_config where mask_main_type = %d";
		sprintf(sql_cmd, fmt_sql, i);
		if (mysql_query(mysql_game, sql_cmd)) {
			LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
			return;
		}

		MYSQL_RES* res = mysql_store_result(mysql_game);
		MYSQL_ROW row;
		int flag = 0;
		while (row = mysql_fetch_row(res)) {
			struct game_mask mask;
			mask.main_type = atoi(row[1]);
			mask.level = atoi(row[2]);
			strcpy(mask.mask_desic, row[3]);
			mask.bonues1 = atoi(row[4]);
			mask.bonues2 = atoi(row[5]);
			mask.bonues3 = atoi(row[6]);
			mask.max_cond = atoi(row[7]);

			vector_push_back(&mask_config->mask_config_set[i-1], &mask);
			flag = 1;
		}

		// 按照任务的等级来排序
		if (flag == 1) {
			void* begin = vector_begin(&mask_config->mask_config_set[i - 1]);
			int size = vector_size(&mask_config->mask_config_set[i - 1]);
			qsort(begin, size, sizeof(struct game_mask), mask_compare);
		}
		// end 
	}
	
	return;
}

void 
connect_to_game() {
	// 连接到中心数据库
	if (mysql_game == NULL) {
		mysql_game = mysql_init(NULL);
		if (!mysql_real_connect(mysql_game,
			GAME_DB_CONF.mysql_ip, GAME_DB_CONF.mysql_name,
			GAME_DB_CONF.mysql_pwd, GAME_DB_CONF.database_name,
			GAME_DB_CONF.mysql_port, NULL, 0)) {
			LOGERROR("connect error!!! \n %s\n", mysql_error(mysql_game));
			mysql_close(mysql_game);
			mysql_game = NULL;

			// 启动一个定时器
			netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_game, NULL, 3.0f);
			// end 
		}
		else {
			LOGINFO("mysql game database connected success !!!!!");
			mysql_set_character_set(mysql_game, "utf8");
			load_game_mask_config(&MASK_CONFIG);
		}
	}
	// end 

	// 连接到redis
	if (redis_game == NULL) {
		struct timeval tv = { 1, 500000 };
		redis_game = redisConnectWithTimeout(GAME_DB_CONF.redis_ip, GAME_DB_CONF.redis_port, tv);
		if (redis_game->err != 0) { // 
			LOGERROR("Connection error: %s\n", redis_game->errstr);
			redisFree(redis_game);
			redis_game = NULL;
			// 连接中心redis失败,启动重连
			if (mysql_game != NULL) {
				// 启动一个定时器
				netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_game, NULL, 3.0f);
			}
			// end 
		}
		else {
			if (GAME_DB_CONF.redis_pwd != NULL) { // 有密码
				redisReply*reply = redisCommand(redis_game, "auth %s", GAME_DB_CONF.redis_pwd);
				if (reply) {
					if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) { // 验证通过
						LOGINFO("Connect game redis sucess!");
						if (GAME_DB_CONF.redis_dbid != 0) {
							if (select_redis_dbid(GAME_DB_CONF.redis_dbid) != 0) {
								redisFree(redis_game);
								redis_game = NULL;
								if (mysql_game != NULL) {
									netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_game, NULL, 3.0f);
								}
							}
						}
					}
					else {
						LOGERROR("Auth Error !!! %s\n", reply->str);
						redisFree(redis_game);
						redis_game = NULL;
						if (mysql_game != NULL) {
							netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_game, NULL, 3.0f);
						}
					}
					freeReplyObject(reply);
				}
			}
			else {
				LOGINFO("Connect game redis sucess!");
				if (GAME_DB_CONF.redis_dbid != 0) {
					if (select_redis_dbid(GAME_DB_CONF.redis_dbid) != 0) {
						redisFree(redis_game);
						redis_game = NULL;
						if (mysql_game != NULL) {
							netbus_add_timer(/*(void(__cdecl *)(void *))*/connect_to_game, NULL, 3.0f);
						}
					}
				}
			}
		}
	}
	// end 
}
int
get_task_bonues_recode_info(unsigned int uid, int taskid, 
                           struct task_bonues_info* info) {
	if (mysql_game == NULL || info == NULL) {
		return -1;
	}

	memset(info, 0, sizeof(struct task_bonues_info));

	char* fmt_sql = "select main_type, mask_level from game_mask_info where id = %d and uid = %d and status = 2 limit 1";
	sprintf(sql_cmd, fmt_sql, taskid, uid);

	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (info) {
			info->main_type = atoi(row[0]);
			info->task_level = atoi(row[1]);
			info->has_info = 1;

			// info
			struct game_mask* begin = vector_begin(&(MASK_CONFIG.mask_config_set[info->main_type - 1]));
			info->bonues_chip = begin[info->task_level - 1].bonues1;
			// end 
		}
		return 0;
	}
	else {
		info->has_info = 0;
	}

	return 0;
}

int
get_login_bonues_info_by_uid(unsigned int uid,
struct login_bonues_info* bonues_info) {
	if (mysql_game == NULL || bonues_info == NULL) {
		return -1;
	}

	memset(bonues_info, 0, sizeof(struct login_bonues_info));

	char* fmt_sql = "select utime, udays, ustatus, ubonues from login_bonueses where uid = %d limit 1";
	sprintf(sql_cmd, fmt_sql, uid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (bonues_info) {
			bonues_info->bonues_time = atoi(row[0]);
			bonues_info->udays = atoi(row[1]);
			bonues_info->never_login_bonues = 0;
			bonues_info->status = atoi(row[2]);
			bonues_info->bonues_value = atoi(row[3]);
		}
		return 0;
	}
	else {
		bonues_info->never_login_bonues = 1;
	}

	return 0;
}

// 第一次获取登陆奖励，所以我们插入
int
insert_login_bonues_info_by_uid(unsigned int uid,
                                struct login_bonues_info* bonues_info) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "insert into login_bonueses(`uid`, `ubonues`, \
`ustatus`, `utime`, `udays`)values(%d, %d, 0, %d, %d)";
	sprintf(sql_cmd, fmt_sql, uid, bonues_info->bonues_value, bonues_info->bonues_time, bonues_info->udays);
	
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}
	return 0;
}

int
update_task_level_complet(unsigned int uid, int main_type, int level) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "update game_mask_info set status = 2, end_time = %d \
where uid = %d and main_type = %d and mask_level = %d";
	sprintf(sql_cmd, fmt_sql, timestamp(), uid, main_type, level);
	
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}
	return 0;
}

int
update_task_on_going(unsigned int uid, int main_type, int level) {
	if (mysql_game == NULL) {
		return -1;
	}
	char* fmt_sql = "update game_mask_info set status = 1 \
					where uid = %d and main_type = %d and mask_level = %d and status = 0";
	sprintf(sql_cmd, fmt_sql, uid, main_type, level);

	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	return 0;
}

int
update_task_cond(unsigned int uid, int main_type) {
	if (mysql_game == NULL) {
		return -1;
	}
	char* fmt_sql = "update game_mask_info set now = now + 1 \
where uid = %d and main_type = %d and status = 1";
	sprintf(sql_cmd, fmt_sql, uid, main_type);

	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	return 0;
}

int
get_task_cond_info(unsigned int uid, int main_type, struct task_cond_info* cond_info) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "select mask_level, now, end_cond from game_mask_info where \
uid = %d and main_type = %d and status = 1 limit 1";
	sprintf(sql_cmd, fmt_sql, uid, main_type);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (cond_info) {
			cond_info->level = atoi(row[0]);
			cond_info->now = atoi(row[1]);
			cond_info->end_cond = atoi(row[2]);
			cond_info->has_info = 1;
		}
		return 0;
	}
	else {
		memset(cond_info, 0, sizeof(struct task_cond_info));
	}
	return 0;
}

int
update_login_bonues_info_by_uid(unsigned int uid,
                                struct login_bonues_info* bonues_info) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "update login_bonueses set ubonues = %d, ustatus = 0, \
utime = %d, udays = %d where uid = %d";

	sprintf(sql_cmd, fmt_sql, bonues_info->bonues_value,
	        bonues_info->bonues_time, bonues_info->udays, uid);

	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}
	return 0;
}

int
update_task_bonues_status(int taskid, int status) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "update game_mask_info set status = %d where id = %d";
	sprintf(sql_cmd, fmt_sql, status, taskid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	return 0;
}

int 
update_login_bonues_status(unsigned int uid, int status) {
	if (mysql_game == NULL) {
		return -1;
	}
	
	char* fmt_sql = "update login_bonueses set ustatus = %d where uid = %d";
	sprintf(sql_cmd, fmt_sql, status, uid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	return 0;
}

int
get_ugame_common_info_by_uid(unsigned int uid, struct ugame_common_info* ugame_info) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "select uchip, uexp, uvip, ustatus from ugame where uid = %d limit 1";
	sprintf(sql_cmd, fmt_sql, uid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		if (ugame_info) {
			ugame_info->uchip = atoi(row[0]);
			ugame_info->uexp = atoi(row[1]);
			ugame_info->uvip = atoi(row[2]);
			ugame_info->ustatus = atoi(row[3]);
			ugame_info->uid = uid;
		}
		return 0;
	}
	return -1;
}

// 获取用户完成的，但是没有领取的信息
// json [[task_id, task_desic], [task_id, task_desic], [task_id, task_desic]]
int
get_user_bonues_info_by_uid(unsigned int uid, json_t** bonues_array) {
	json_t* bonues_set = json_new_array();
	*bonues_array = bonues_set;

	if (mysql_game == NULL) {
		return -1;
	}
	char* fmt_sql = "select id, main_type, mask_level \
from game_mask_info where uid = %d and status = 2";
	sprintf(sql_cmd, fmt_sql, uid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row;

	while (row = mysql_fetch_row(res)) {
		json_t* one_recode = json_new_array();
		json_array_push_number(one_recode, atoi(row[0])); // task_id
		int main_type = atoi(row[1]);
		int level = atoi(row[2]);

		struct vector_array* mask_desic = &(MASK_CONFIG.mask_config_set[main_type - 1]);
		struct game_mask* mask = vector_begin(mask_desic);
		json_array_push_string(one_recode, mask[level - 1].mask_desic);

		json_insert_child(bonues_set, one_recode);
	}

	return 0;
}

// 获取用户正在进行的任务
// [[task_id, task_desic], [task_id, task_desic], [task_id, task_desic]]
int
get_user_mask_info_by_uid(unsigned int uid, json_t** mask_array) {

	json_t* mask_set = json_new_array();
	*mask_array = mask_set;

	if (mysql_game == NULL) {
		return -1;
	}
	char* fmt_sql = "select id, main_type, mask_level \
					from game_mask_info where uid = %d and status = 1";
	sprintf(sql_cmd, fmt_sql, uid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row;

	while (row = mysql_fetch_row(res)) {
		json_t* one_recode = json_new_array();
		json_array_push_number(one_recode, atoi(row[0])); // task_id
		int main_type = atoi(row[1]);
		int level = atoi(row[2]);

		struct vector_array* mask_desic = &(MASK_CONFIG.mask_config_set[main_type - 1]);
		struct game_mask* mask = vector_begin(mask_desic);
		json_array_push_string(one_recode, mask[level - 1].mask_desic);

		json_insert_child(mask_set, one_recode);
	}

	return 0;
}

int
insert_ugame_common_info_by_uid(unsigned int uid, struct ugame_common_info* info) {
	if (mysql_game == NULL) {
		return -1;
	}
	char* fmt_sql = "insert into ugame(`uid`, `uchip`, \
`uexp`, `ustatus`)values(%d, %d, %d, 0)";
	sprintf(sql_cmd, fmt_sql, uid, info->uchip, info->uexp);

	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	return 0;
}

int
check_task_is_exist(unsigned int uid, int main_type, int task_level) {
	char* fmt_sql = "select id from game_mask_info where uid = %d and main_type = %d and mask_level = %d";
	sprintf(sql_cmd, fmt_sql, uid, main_type, task_level);

	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return 1;
	}
	MYSQL_RES* res = mysql_store_result(mysql_game);
	MYSQL_ROW row = mysql_fetch_row(res);
	if (row) {
		return 1;
	}

	return 0;
}

// 为这个玩家插入任务
int
insert_game_task(unsigned int uid) {
	if (mysql_game == NULL) {
		return -1;
	}

	for (int type = 1; type <= MASK_NUM; type++) {
		int size = vector_size(&MASK_CONFIG.mask_config_set[type - 1]);
		
		struct game_mask* begin = vector_begin(&MASK_CONFIG.mask_config_set[type - 1]);
		for (int i = 0; i < size; i++) {
			// 判断一下，这里的任务与等级是否存在，如果存在continue;
			if (check_task_is_exist(uid, begin[i].main_type, begin[i].level)) {
				continue;
			}
			// end 

			int status = (i == 0) ? 1 : 0;
			char* fmt_sql = "insert into game_mask_info(`uid`, `main_type`, \
`mask_level`, `now`, `end_cond`, `status`)values(%d, %d, %d, 0, %d, %d)";
			sprintf(sql_cmd, fmt_sql, uid, begin[i].main_type, begin[i].level, 
				begin[i].max_cond, status);

			if (mysql_query(mysql_game, sql_cmd)) {
				LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
				return -1;
			}
		}
	}
	return 0;
}

int
add_ugame_uchip(unsigned int uid, int chip) {
	if (mysql_game == NULL) {
		return -1;
	}

	char* fmt_sql = "update ugame set uchip = uchip + %d where uid = %d";
	sprintf(sql_cmd, fmt_sql, chip, uid);
	if (mysql_query(mysql_game, sql_cmd)) {
		LOGERROR("%s %s\n", sql_cmd, mysql_error(mysql_game));
		return -1;
	}

	return 0;
}


void
set_ugame_common_info_inredis(struct ugame_common_info* info) {
	if (redis_game == NULL) {
		return;
	}

	char* fmt_redis_cmd = "HMSET ugame_common_%d uid %d uchip %d uexp %d uvip %d ustatus %d ";
	redisReply*reply = redisCommand(redis_game, fmt_redis_cmd, info->uid, info->uid, info->uchip, info->uexp, info->uvip, info->ustatus);
	if (reply) {
		if (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) {
			LOGINFO("set ugame common info in redis %d success\n", info->uid);
		}
		else {
			LOGERROR("set uinfo in redis error %s\n", reply->str);
		}
		freeReplyObject(reply);
	}


}

// 金币排行的key  SLICE_FRUIT_CHIP_RANK
#define RANK_CHIP_KEY "SLICE_FRUIT_CHIP_RANK"

void
flush_game_rank_info_inredis(struct ugame_common_info* info) {
	if (redis_game == NULL) {
		return;
	}

	// key, 权重, rank_chip_uid
	char* fmt_redis_cmd = "ZADD %s %d %d";
	redisReply*reply = redisCommand(redis_game, fmt_redis_cmd, RANK_CHIP_KEY, -(info->uchip), info->uid);
	if (reply) {
		if (reply->type == REDIS_REPLY_INTEGER) {
			LOGINFO("flush game rank info in redis %d success\n", info->uid);
		}
		else {
			LOGERROR("flush game rank info in redis error %s\n", reply->str);
		}
		freeReplyObject(reply);
	}

}

int
get_game_rank_user_inredis(struct game_rank_user* rank_user) {
	if (rank_user == NULL || redis_game == NULL) {
		return -1;
	}
	memset(rank_user, 0, sizeof(struct game_rank_user));


	int ret = -1;
	int stop = MAX_RANK_OPTION;
	char* fmt_redis_cmd = "zrange %s 0 %d";
	redisReply*reply = redisCommand(redis_game, fmt_redis_cmd, RANK_CHIP_KEY, stop);
	if (reply) {
		if (reply->type == REDIS_REPLY_ARRAY) {
			int num = (reply->elements > MAX_RANK_OPTION) ? MAX_RANK_OPTION : reply->elements;

			rank_user->rank_num = num;
			for (int i = 0; i < num; i++) {
				rank_user->rank_user_uid[i] = atoi(reply->element[i]->str);
			}
			ret = 0;
		}

		freeReplyObject(reply);
	}

	return ret;
}

int
get_ugame_common_info_inredis(unsigned uid, struct ugame_common_info* uinfo) {
	if (uinfo == NULL || redis_game == NULL) {
		return -1;
	}
	int ret = -1;
	char* fmt_redis_cmd = "hgetall ugame_common_%d";
	redisReply*reply = redisCommand(redis_game, fmt_redis_cmd, uid);
	if (reply) {
		if (reply->type == REDIS_REPLY_ARRAY) { // 查询结果是一个数组
			if (reply->elements == 10) {
				uinfo->uid = uid;
				uinfo->uchip = atoi(reply->element[3]->str);
				uinfo->uexp = atoi(reply->element[5]->str);
				uinfo->uvip = atoi(reply->element[7]->str);
				uinfo->ustatus = atoi(reply->element[9]->str);
				ret = 0;
			}
			else {
				LOGERROR("get ugame_common_info error:%d, eleme count %d\n", uid, reply->elements);
			}
		}
		else {
			if (reply->str) {
				LOGERROR("get ugame_common_info error:%d, %s\n", uid, reply->str);
			}
		}
		freeReplyObject(reply);
	}
	return ret;
}
