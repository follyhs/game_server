#ifndef __TT_VECTOR_ARRAY_H__
#define __TT_VECTOR_ARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif

	struct vector_array {
		int alloc_count;
		unsigned char* _data;

		int elem_count;
		int elem_size;
	};

	void vector_define(struct vector_array* v, int size_of_elem);
	void vector_clear(struct vector_array* v);

	void vector_pop_all(struct vector_array* v);
	
	void vector_push_back(struct vector_array* v, void* elem_ptr);
	void vector_pop_back(struct vector_array* v, void* elem_ptr);

	void vector_push_front(struct vector_array* v, void* elem_ptr);
	void vector_pop_front(struct vector_array* v, void*elem_ptr);

	void vector_erase(struct vector_array* v, int index);
	void* vector_iterator_at(struct vector_array* v, int index);
	int vector_size(struct vector_array* v);
	
	void* vector_begin(struct vector_array* v);
	void* vector_end(struct vector_array* v);

#ifdef __cplusplus
}
#endif

#endif
