#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vector_array.h"

#define my_malloc malloc
#define my_free free
#define my_realloc realloc

#define ALLOC_STEP 32

void vector_define(struct vector_array* v, int size_of_elem) {
	memset(v, 0, sizeof(struct vector_array));
	v->elem_size = size_of_elem;
}

void vector_clear(struct vector_array* v) {
	if (v->_data != NULL) {
		my_free(v->_data);
		v->_data = NULL;
	}
}

void vector_pop_all(struct vector_array* v) {
	v->elem_count = 0;
}

void vector_push_back(struct vector_array* v, void* elem_ptr) {
	if (v->elem_count >= v->alloc_count) {
		v->alloc_count += ALLOC_STEP;
		v->_data = (unsigned char*)my_realloc(v->_data, (v->alloc_count * v->elem_size));
	}

	memcpy(v->_data + v->elem_count * v->elem_size, elem_ptr, v->elem_size);
	v->elem_count ++;
}

void vector_pop_back(struct vector_array* v, void* elem_ptr) {
	if (v->elem_count <= 0) {
		v->elem_count = 0;
		return;
	}

	v->elem_count --;
	if (elem_ptr) {
		memcpy(elem_ptr, v->_data + v->elem_count * v->elem_size, v->elem_size);
	}
}

void vector_push_front(struct vector_array* v, void* elem_ptr) {
	if ((v->elem_count + 1) >= v->alloc_count) {
		v->alloc_count += ALLOC_STEP;
		v->_data = (unsigned char*)my_realloc(v->_data, (v->alloc_count * v->elem_size));
	}

	if (v->elem_count > 0) {
		memmove(v->_data + v->elem_size, v->_data, v->elem_count * v->elem_size);
	}
	
	if (elem_ptr) {
		memcpy(v->_data, elem_ptr, v->elem_size);
	}
}

void vector_pop_front(struct vector_array* v, void*elem_ptr) {
	if (elem_ptr) {
		memcpy(elem_ptr, v->_data, v->elem_size);
	}
	
	v->elem_count --;
	if (v->elem_count > 0) {
		memmove(v->_data, v->_data + v->elem_size, v->elem_count * v->elem_size);
	}
}

void vector_erase(struct vector_array* v, int index) {
	if (v->elem_count == 0 || index < 0 || index >= v->elem_count) {
		return;
	}

	if (index == v->elem_count - 1) {
		v->elem_count --;
		return;
	}

	unsigned char* dst = v->_data + index * v->elem_size;
	unsigned char* src = dst + v->elem_size;
	v->elem_count --;
	if (v->elem_count <= 0) {
		return;
	}

	int count = v->elem_count - index;
	memmove(dst, src, count * v->elem_size);
}

void* vector_iterator_at(struct vector_array* v, int index) {
	if (index < 0 || index >= v->elem_count) {
		return NULL;
	}

	return v->_data + v->elem_size * index;
}

int vector_size(struct vector_array* v) {
	return v->elem_count;
}

void* vector_end(struct vector_array* v) {
	if (v->elem_count <= 0) {
		return NULL;
	}
	return (v->_data + (v->elem_count - 1) * v->elem_size);
}

void* vector_begin(struct vector_array* v) {
	if (v->elem_count <= 0) {
		return NULL;
	}
	return v->_data;
}