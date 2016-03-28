#pragma once
#include "Data/DataType.h"
#include "Allocator.h"
#include "Debug/C3Debug.h"
#include <memory.h>

struct MemoryBlock {
  void* data;
  u32 size;
};

typedef void(*ReleaseFn)(void* ptr, void* user_data);

struct MemoryRef {
  MemoryBlock mem;
  ReleaseFn release_fn;
  void* user_data;
};

bool is_mem_ref(const MemoryBlock* mem);
inline const MemoryBlock* mem_alloc(u32 size) {
  MemoryBlock* mem = (MemoryBlock*)C3_ALLOC(g_allocator, sizeof(MemoryBlock) + size);
  mem->size = size;
  mem->data = (u8*)mem + sizeof(MemoryBlock);
  //c3_log("alloc %d\n", mem->size);
  return mem;
}
inline const MemoryBlock* mem_copy(const void* data, u32 size) {
  const MemoryBlock* mem = mem_alloc(size);
  ::memcpy(mem->data, data, size);
  return mem;
}
inline const MemoryBlock* mem_clone(const MemoryBlock* mem) {
  return mem_copy(mem->data, mem->size);
}
inline void mem_free(const MemoryBlock* mem) {
  c3_assert_return(mem);
  if (is_mem_ref(mem)) {
    MemoryRef* ref = (MemoryRef*)mem;
    if (ref->release_fn) ref->release_fn(mem->data, ref->user_data);
  }// else { c3_log("free %d\n", mem->size); }
  C3_FREE(g_allocator, const_cast<MemoryBlock*>(mem));
}

inline const MemoryBlock* mem_ref(const void* data, u32 size, ReleaseFn release_fn = nullptr, void* user_data = nullptr) {
	MemoryRef* ref = (MemoryRef*)C3_ALLOC(g_allocator, sizeof(MemoryRef));
	ref->mem.size = size;
	ref->mem.data = (void*)data;
	ref->release_fn = release_fn;
	ref->user_data = user_data;
	return &ref->mem;
}

inline bool is_mem_ref(const MemoryBlock* mem) {
  return mem->data != (u8*)mem + sizeof(MemoryBlock);
}

void mem_init();
