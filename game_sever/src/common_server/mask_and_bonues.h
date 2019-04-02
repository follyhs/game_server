#ifndef __MASK_AND_BONUES_H__
#define __MASK_AND_BONUES_H__

#include "../netbus/netbus.h"

void
get_mask_and_bonues_info(void* module_data, json_t* json,
                          struct session* s,
						unsigned int uid, unsigned int s_key);

void
task_and_bonues_win_one_round(void* module_data, json_t* json,
                              struct session* s,
						       unsigned int uid, unsigned int s_key);

void 
recv_task_bonues(void* module_data, json_t* json,
                 struct session* s,
                 unsigned int uid, unsigned int s_key);
						  
#endif

