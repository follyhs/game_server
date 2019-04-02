#ifndef __LOGIN_BONUESES_H__
#define __LOGIN_BONUESES_H__

#include "../netbus/netbus.h"

void 
check_login_bonues(void* module_data, json_t* json,
                   struct session* s,
				   unsigned int uid, unsigned int s_key);

void 
get_login_bonues_info(void* module_data, json_t* json,
                      struct session* s,
                      unsigned int uid, unsigned int s_key);

void 
recv_login_bonues(void* module_data, json_t* json,
                      struct session* s,
                      unsigned int uid, unsigned int s_key);

#endif

