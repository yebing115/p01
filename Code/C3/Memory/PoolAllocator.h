#pragma once

#include "Platform/PlatformSync.h"
#include "Allocator.h"

template <bool THREAD_SAFE>
class BasePoolAllocator: public IAllocator {
public:
  BasePoolAllocator()
  : _data(nullptr), _size(0), _obj_size(0), _free_list(nullptr), _used_size(0) {}
  void Init(void* data, size_t size, size_t obj_size) {
    _data = data;
    _size = size;
    _obj_size = obj_size;
    _used_size = 0;
    _free_list = (void**)_data;
    void** p = _free_list;
    size_t n = size / obj_size;
    for (int i = 0; i < n - 1; ++i) {
      *p = (void*)((u8*)p + obj_size);
      p = (void**)p;
    }
    *p = nullptr;
    if (THREAD_SAFE) _lock.Reset();
  }
  ~BasePoolAllocator() {
    _free_list = nullptr;
  }
  void* Alloc(size_t size, size_t align, const char* file, u32 line) override {
    c3_assert(size <= _obj_size);
    if (THREAD_SAFE) _lock.Lock();
    if (!_free_list) return nullptr;
    void* p = _free_list;
    _free_list = (void**)*_free_list;
    _used_size += _obj_size;
    if (THREAD_SAFE) _lock.Unlock();
    return p;
  }

  void Free(void* ptr, size_t align, const char* file, u32 line) override {
    if (!ptr) return;
    if (THREAD_SAFE) _lock.Lock();
    *(void**)ptr = _free_list;
    _free_list = (void**)ptr;
    _used_size -= _obj_size;
    if (THREAD_SAFE) _lock.Unlock();
  }

  void* Realloc(void* ptr, size_t size, size_t align, const char* file, u32 line) override {
    Free(ptr, align, file, line);
    if (size > 0) return Alloc(size, align, file, line);
    else return nullptr;
  }
  void* GetPool() const { return _data; }
private:
  void* _data;
  size_t _size;
  size_t _obj_size;
  void** _free_list;
  size_t _used_size;
  SpinLock _lock;
};

typedef BasePoolAllocator<false> PoolAllocator;
typedef BasePoolAllocator<true> ThreadSafePoolAllocator;
