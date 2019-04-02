#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "json.h"

void 
json_free_str(char* json_str) {
	free(json_str);
}

int
json_object_size(json_t* parent) {
	int count = 0;
	json_t* walk = parent->child;
	while (walk) {
		count++;
		walk = walk->next;
	}
	return count;
}

int
json_array_size(json_t* parent) {
	int count = 0;
	json_t* walk = parent->child;
	while (walk) {
		count++;
		walk = walk->next;
	}
	return count;
}

json_t*
json_object_at(json_t* parent, char* key) {
	json_t* j_key = json_find_first_label(parent, key);
	if (!j_key) {
		return NULL;
	}

	return j_key->child;
}

json_t*
json_array_at(json_t* parent, int i) {
	json_t* j_key = parent->child;
	int index = 0;
	while (j_key) {
		if (index == i) {
			return j_key;
		}
		j_key = j_key->next;
	}

	return NULL;
}

void
json_object_push_number(json_t* root, char*key, int value) {
	char buf[16];
	sprintf(buf, "%d", value);
	
	json_t* j_value = json_new_number(buf);
	json_insert_pair_into_object(root, key, j_value);
}

void
json_object_push_string(json_t* parent, char* key, char* value) {
	json_t* j_value = json_new_string(value);
	json_insert_pair_into_object(parent, key, j_value);
}

void
json_array_push_number(json_t* parent, int value) {
	char buf[16];
	sprintf(buf, "%d", value);
	
	json_t* j_value = json_new_number(buf);
	json_insert_child(parent, j_value);
}

void
json_array_push_string(json_t* parent, char* value) {
	json_t* j_value = json_new_string(value);
	json_insert_child(parent, j_value);
}

json_t*
json_new_command(int stype, int command) {
	json_t* root = json_new_object();
	json_object_push_number(root, "0", stype);
	json_object_push_number(root, "1", command);

	return root;
}

json_t*
json_new_int_array(int* value, int count) {
	json_t* json = json_new_array();
	for (int i = 0; i < count; i++) {
		json_array_push_number(json, value[i]);
	}
	return json;
}

void
json_object_update_number(json_t* parent, char* key, int number) {
	char buf[64];
	sprintf(buf, "%d", number);
	json_t* j_value = json_object_at(parent, key);
	if (j_value == NULL || j_value->type != JSON_NUMBER) {
		return;
	}
	if (j_value->text) {
		free(j_value->text);
	}
	
	int len = (strlen(buf) + 1);
	j_value->text = malloc(len);
	strncpy(j_value->text, buf, len);
}

int
json_object_get_number(json_t* json, char* key) {
	json_t* j_value = json_object_at(json, key);
	if (j_value == NULL || j_value->type != JSON_NUMBER) {
		return 0;
	}

	return atoi(j_value->text);
}

char*
json_object_get_string(json_t* json, char* key) {
	json_t* j_value = json_object_at(json, key);
	if (j_value == NULL || j_value->type != JSON_STRING) {
		return NULL;
	}

	return j_value->text;
}

void
json_object_remove(json_t* json, char* key) {
	json_t* j_key = json_find_first_label(json, key);
	json_free_value(&j_key);
}
