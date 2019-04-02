#ifndef __AUTH_H__
#define __AUTH_H__

#include "../netbus/netbus.h"

void
guest_login(void* module_data, json_t* json,
            struct session* s,
			unsigned int uid, unsigned int s_key);

void
user_login(void* module_data, json_t* json,
           struct session* s,
		   unsigned int uid, unsigned int s_key);

void
edit_user_profile(void* module_data, json_t* json,
                  struct session* s,
			      unsigned int uid, unsigned int s_key);

void
guest_upgrade_account(void* module_data, json_t* json,
                  struct session* s,
			      unsigned int uid, unsigned int s_key);

void
user_modify_upwd(void* module_data, json_t* json,
                 struct session* s,
                 unsigned int uid, unsigned int s_key);

#endif

