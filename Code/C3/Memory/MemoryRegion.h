#pragma once
#include "Data/DataType.h"
#include "Allocator.h"
#include "Debug/C3Debug.h"
#include <memory.h>

struct MemoryRegion {
  void* data;
  u32 size;

  MemoryRegion() : data(nullptr), size(0) {}
  MemoryRegion(const void* d, u32 s) : data((void*)d), size(s) {}
  virtual ~MemoryRegion() = 0;
};
inline MemoryRegion::~MemoryRegion() {}

typedef void(*ReleaseFn)(void* ptr, u32 size, void* user_data);

struct MemoryRef : public MemoryRegion {
  ReleaseFn release_fn;
  void* user_data;

  ~MemoryRef() {
    if (release_fn) release_fn(data, size, user_data);
  }
};

struct ResizableMemoryRegion : public MemoryRegion {
  ~ResizableMemoryRegion() { Resize(0); }
  virtual void Resize(u32 new_size) = 0;
};
inline void ResizableMemoryRegion::Resize(u32 new_size) {}

struct AllocatedMemory : public ResizableMemoryRegion {
  IAllocator* allocator;

  AllocatedMemory(IAllocator* a) : allocator(a) {}
  void Resize(u32 new_size) override {
    data = C3_REALLOC(allocator, data, new_size);
    size = data ? new_size : 0;
  }
};

inline AllocatedMemory* mem_alloc(u32 size) {
  auto mem = ::new AllocatedMemory(g_allocator);
  mem->data = C3_ALLOC(g_allocator, size);
  mem->size = size;
  return mem;
}
inline AllocatedMemory* mem_copy(const void* data, u32 size) {
  auto mem = mem_alloc(size);
  ::memcpy(mem->data, data, size);
  return mem;
}
inline const AllocatedMemory* mem_clone(const MemoryRegion* mem) {
  return mem_copy(mem->data, mem->size);
}
inline void mem_free(const MemoryRegion* mem) {
  delete mem;
}

inline MemoryRef* mem_ref(const void* data, u32 size, ReleaseFn release_fn = nullptr, void* user_data = nullptr) {
  auto ref = ::new MemoryRef;
  ref->data = (void*)data;
  ref->size = size;
	ref->release_fn = release_fn;
	ref->user_data = user_data;
	return ref;
}

void mem_init();
