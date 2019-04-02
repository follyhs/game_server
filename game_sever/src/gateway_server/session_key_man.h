#ifndef __SESSION_KEY_MAN_H__
#define __SESSION_KEY_MAN_H__

struct session;

void 
init_session_key_man();
void 
exit_session_key_man();

unsigned int 
get_key_from_session_map();

void 
save_session_with_key(unsigned int key, struct session* s);

void
clear_session_with_key(unsigned int key);

struct session* 
get_session_with_key(unsigned int key);

void
clear_session_with_value(struct session* s);

struct session*
get_session_with_uid(unsigned int uid);

void
save_session_with_uid(unsigned int uid, struct session* s);

void
clear_session_with_uid(unsigned int uid);

#endif

