
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "game_command.h"
#include "game_stype.h"
#include "netbus/netbus.h"

void
json_get_command_tag(json_t* cmd, unsigned int* uid, unsigned int* s_key) {
	*uid = (unsigned int)json_object_get_number(cmd, "uid");
	*s_key = (unsigned int)json_object_get_number(cmd, "s_key");
}

json_t*
json_new_server_return_cmd(int stype, int cmd,
                           unsigned int uid,
						   unsigned int s_key) {
	json_t* json = json_new_command(stype + STYPE_RETURN, cmd);

	json_object_push_number(json, "s_key", s_key);
	json_object_push_number(json, "uid", uid);
	return json;
}

void
write_error(struct session* s, int stype, 
            int cmd, int status,
			unsigned int uid, unsigned int s_key) {
	json_t* json = json_new_server_return_cmd(stype, cmd, uid, s_key);
	json_object_push_number(json, "2", status);
	session_send_json(s, json);
	json_free_value(&json);
}