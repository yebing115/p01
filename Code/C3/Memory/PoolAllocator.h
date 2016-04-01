#pragma once

#include "Platform/PlatformSync.h"
#include "Allocator.h"

template <bool THREAD_SAFE>
class BasePoolAllocator: public IAllocator {
public:
  BasePoolAllocator()
  :  _obj_size(0), _num(0), _align(0), _allocator(nullptr), _free_list(nullptr), _used_nums(0) {}
  void Init(size_t obj_size, size_t align, size_t num, IAllocator* allocator = g_allocator) {
    _obj_size = ALIGN_MASK(obj_size, align - 1);
    _align = align;
    _num = num;
    _data = C3_ALIGNED_ALLOC(allocator, _obj_size * num, align);
    _used_nums = 0;
    _free_list = (void**)_data;
    void** p = _free_list;
    for (size_t i = 0; i < _num - 1; ++i) {
      *p = (void*)((u8*)p + obj_size);
      p = (void**)(*p);
    }
    *p = nullptr;
  }
  ~BasePoolAllocator() {
    _free_list = nullptr;
    C3_ALIGNED_FREE(_allocator, _data, _align);
  }
  void* Alloc(size_t size, size_t align, const char* file, u32 line) override {
    c3_assert(size <= _obj_size);
    if (THREAD_SAFE) _lock.Lock();
    if (!_free_list) return nullptr;
    void* p = _free_list;
    _free_list = (void**)*_free_list;
    ++_used_nums;
    if (THREAD_SAFE) _lock.Unlock();
    return p;
  }

  void Free(void* ptr, size_t align, const char* file, u32 line) override {
    if (!ptr) return;
    if (THREAD_SAFE) _lock.Lock();
    *(void**)ptr = _free_list;
    _free_list = (void**)ptr;
    --_used_nums;
    if (THREAD_SAFE) _lock.Unlock();
  }

  void* Realloc(void* ptr, size_t size, size_t align, const char* file, u32 line) override {
    Free(ptr, align, file, line);
    if (size > 0) return Alloc(size, align, file, line);
    else return nullptr;
  }
  void* GetData() const { return _data; }
private:
  size_t _obj_size;
  size_t _num;
  size_t _align;
  IAllocator* _allocator;
  void* _data;
  void** _free_list;
  size_t _used_nums;
  SpinLock _lock;
};

typedef BasePoolAllocator<false> PoolAllocator;
typedef BasePoolAllocator<true> ThreadSafePoolAllocator;
