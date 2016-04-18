#pragma once

#include "Platform/PlatformSync.h"
#include "Allocator.h"

class LinearAllocator : public IAllocator {
public:
  LinearAllocator(IAllocator* allocator = g_allocator)
  : _allocator(allocator), _size(0), _used(0) {}
  void Init(size_t size, IAllocator* allocator = g_allocator) {
    _data = C3_ALLOC(allocator, size);
    _size = size;
    _used = 0;
  }
  ~LinearAllocator() {
    C3_FREE(_allocator, _data);
  }
  void* Alloc(size_t size, size_t align, const char* file, u32 line) override {
    u32 real_size = ALIGN_MASK(size, align - 1);
    u32 old;
    do {
      old = _used;
      if (old + real_size > _size) return nullptr;
    } while (!_used.compare_exchange_strong(old, old + real_size));
    return (u8*)_data + ALIGN_MASK(old, align - 1);
  }

  void Free(void* ptr, size_t align, const char* file, u32 line) override {}

  void* Realloc(void* ptr, size_t size, size_t align, const char* file, u32 line) override {
    if (size > 0) return Alloc(size, align, file, line);
    else return nullptr;
  }
  void* GetData() const { return _data; }
  void Reset() { _used = 0; }
private:
  IAllocator* _allocator;
  u32 _size;
  atomic_u32 _used;
  void* _data;
};
