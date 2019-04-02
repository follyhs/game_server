#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "session_key_man.h"
#include "../utils/hash_int_map.h"

struct {
	unsigned int key_id;
	struct hash_int_map* key_session_map;

	struct hash_int_map* uid_session_map;
}SESSION_MAP;

void
init_session_key_man() {
	SESSION_MAP.key_session_map = create_hash_int_map();
	SESSION_MAP.key_id = 1;

	SESSION_MAP.uid_session_map = create_hash_int_map();
}

unsigned int
get_key_from_session_map() {
	unsigned int key = SESSION_MAP.key_id++;
	return key;
}

void
exit_session_key_man() {
	destroy_hash_int_map(SESSION_MAP.key_session_map);
	destroy_hash_int_map(SESSION_MAP.uid_session_map);
}

void
save_session_with_key(unsigned int key, struct session* s) {
	set_hash_int_map(SESSION_MAP.key_session_map, key, s);
}

void
clear_session_with_key(unsigned int key) {
	remove_hash_int_key(SESSION_MAP.key_session_map, key);
}

void
clear_session_with_value(struct session* s) {
	remove_hash_int_value(SESSION_MAP.key_session_map, s);
}

struct session*
get_session_with_key(unsigned int key) {
	return (struct session*)get_value_with_key(SESSION_MAP.key_session_map, key);
}

struct session*
get_session_with_uid(unsigned int uid) {
	return (struct session*)get_value_with_key(SESSION_MAP.uid_session_map, uid);
}

void
save_session_with_uid(unsigned int uid, struct session* s) {
	set_hash_int_map(SESSION_MAP.uid_session_map, uid, s);
}

void
clear_session_with_uid(unsigned int uid) {
	remove_hash_int_key(SESSION_MAP.uid_session_map, uid);
}

