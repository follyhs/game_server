#ifndef __HASH_INT_MAP__
#define __HASH_INT_MAP__

struct hash_int_map;

struct hash_int_map*
create_hash_int_map();

void
destroy_hash_int_map(struct hash_int_map* map);

void
set_hash_int_map(struct hash_int_map* map, unsigned int key, void* value);

void
remove_hash_int_key(struct hash_int_map* map, unsigned int key);

void
remove_hash_int_value(struct hash_int_map* map, void*value);

void* 
get_value_with_key(struct hash_int_map* map, unsigned int key);

// 如果通过了匹配条件函数，那么就返回这个对象，
// 否者返回NULL
void* for_each_find(struct hash_int_map* map, 
	int(*find_cond)(void* value, void* udata), void* udata);
#endif
