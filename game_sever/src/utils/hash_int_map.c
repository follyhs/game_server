#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hash_int_map.h"

#define my_malloc malloc
#define my_free free

#define HASH_AREA 1024

struct hash_node {
	unsigned int key;
	void* value;
	struct hash_node* next;
};

struct hash_int_map {
	struct hash_node* hash_set[HASH_AREA];
};

struct hash_int_map*
create_hash_int_map() {
	struct hash_int_map* map = my_malloc(sizeof(struct hash_int_map));
	memset(map, 0, sizeof(struct hash_int_map));

	return map;
}

void
destroy_hash_int_map(struct hash_int_map* map) {
	// 释放所有节点
	for (int index = 0; index < HASH_AREA; index++) {
		struct hash_node** walk = &map->hash_set[index];
		while (*walk) {
			struct hash_node* node = (*walk);
			*walk = node->next;
			node->next = NULL;
			my_free(node);
		}
	}
	// end 
	my_free(map);
}

void
set_hash_int_map(struct hash_int_map* map, 
                 unsigned int key, void* value) {
	int index = key % HASH_AREA;
	struct hash_node** walk = &map->hash_set[index];

	while (*walk) {
		if ((*walk)->key == key) { // 已经存在的key,所以覆盖，然后返回
			(*walk)->value = value;
			return;
		}
		walk = &((*walk)->next);
	}

	struct hash_node* node = my_malloc(sizeof(struct hash_node));
	memset(node, 0, sizeof(struct hash_node));

	node->key = key;
	node->value = value;
	node->next = NULL;
	*walk = node;
	
}

void
remove_hash_int_key(struct hash_int_map* map, unsigned int key) {
	int index = key % HASH_AREA;
	struct hash_node** walk = &map->hash_set[index];
	while (*walk) {
		if ((*walk)->key == key) {
			struct hash_node* node = (*walk);
			*walk = node->next;
			node->next = NULL;
			my_free(node);
			return;
		}

		walk = &(*walk)->next;
	}
}

void
remove_hash_int_value(struct hash_int_map* map, void*value) {
	for (int index = 0; index < HASH_AREA; index++) {
		struct hash_node** walk = &map->hash_set[index];
		while (*walk) {
			if ((*walk)->value == value) {
				struct hash_node* node = (*walk);
				*walk = node->next;
				node->next = NULL;
				my_free(node);
				// return;
			}
			else {
				walk = &(*walk)->next;
			}
		}
	}
}

void*
get_value_with_key(struct hash_int_map* map, unsigned int key) {
	int index = key % HASH_AREA;
	struct hash_node** walk = &map->hash_set[index];
	while (*walk) {
		if ((*walk)->key == key) {
			return (*walk)->value;
		}
		walk = &(*walk)->next;
	}

	return NULL;
}

void* 
for_each_find(struct hash_int_map* map,
              int(*find_cond)(void* value, void* udata), void* udata) {
	if (find_cond == NULL) {
		return NULL;
	}

	for (int index = 0; index < HASH_AREA; index++) {
		struct hash_node* walk = map->hash_set[index];
		while (walk) {
			if (find_cond && find_cond(walk->value, udata)) {
				return walk->value;
			}
			walk = walk->next;
		}
	}

	return NULL;
}