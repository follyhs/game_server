#ifndef __CENTER_DATABASE_H__
#define __CENTER_DATABASE_H__

struct user_info{
	unsigned int uid;
	char unick[64];
	char phone_number[64];
	int uface;
	int usex;
	int is_guest;
};

// extern void* mysql_center;
void connect_to_center();

// -1, 操作失败，0,表示我们操作成功
int 
get_uinfo_by_ukey(char* ukey, struct user_info* uinfo);

int
get_uinfo_by_uid(unsigned int uid, struct user_info* uinfo);

int
get_uinfo_by_uname_upwd(char* uname, char* upwd, struct user_info* uinfo);

// 插入一个游客账号
int
insert_guest_with_ukey(char* ukey, char* unick,
                       int uface, int usex);


int
update_user_profile(unsigned int uid, char* unick, int uface, int usex);

int
update_user_upwd(unsigned int uid, char* old_upwd_md5, char* new_upwd_md5);
// 为0，表示没有占用，-1,表示占用了
int
is_uname_exist(unsigned int uid, char* uname);

// 0, 表示成功，-1,表示失败
int 
upgrade_account(unsigned int uid, char* uname, char* upwd_md5);

// 将我们从数据库读入的uinfo保存到我们的redis里面去
void 
set_uinfo_inredis(struct user_info* uinfo);
// 为0，表示正常，否者获取失败
int 
get_uinfo_inredis(unsigned int uid, struct user_info* uinfo);



#endif

