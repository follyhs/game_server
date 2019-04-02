#ifndef __CACHE_ALLOC_H__
#define __CACHE_ALLOC_H__

// 创建这样一个内存分配池
// 参数1: 元素的个数
// 参数2: 每个元素的大小
struct cache_alloc*
create_cache_alloc(int elem_count, int elem_size);
// 销毁这个内存池
void 
destroy_cache_alloc(struct cache_alloc* cache);

// 从内存池里面分配一个元素
void* 
cache_alloc(struct cache_alloc* cache, int elem_size);

void
cache_free(struct cache_alloc* cache, void* elem_ptr);


#endif

