#ifndef __UGAME_COMMON_H__
#define __UGAME_COMMON_H__

#include "../netbus/netbus.h"

void
get_ugame_common_info(void* module_data, json_t* json,
                      struct session* s,
				      unsigned int uid, unsigned int s_key);

void
get_ugame_rank_info(void* module_data, json_t* json,
                      struct session* s,
				      unsigned int uid, unsigned int s_key);


					  
#endif

