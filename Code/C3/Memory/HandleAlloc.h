#pragma once
#include "Data/DataType.h"
#include "Debug/C3Debug.h"
#include "Pattern/Handle.h"

template <HandleType TYPE, u32 COUNT>
class HandleAlloc {
public:
  HandleAlloc(): _used(0) {
    for (u32 i = 0; i < COUNT; ++i) {
      auto& h = _handles[i];
      h.idx = i;
      h.age = 0;
      h.type = TYPE;
      _indices[i] = i;
    }
  }
  void Clear() { _used = 0; }
  Handle<TYPE> Alloc() {
    _spin_lock.Lock();
    c3_assert_return_x(_used < COUNT, Handle<TYPE>());
    ++_used;
    _spin_lock.Unlock();
    return _handles[_used - 1];
  }
  void Free(Handle<TYPE> h) {
    //c3_assert(h.idx < COUNT);
    //c3_assert(_indices[h.idx] < _used);
    //c3_assert(_handles[h.idx].age == h.age);
    _spin_lock.Lock();
    if (_indices[h.idx] != _used - 1) {
      u32 old_index = _indices[h.idx];
      swap(_indices[h.idx], _indices[_handles[_used - 1].idx]);
      swap(_handles[old_index], _handles[_used - 1]);
    }
    --_used;
    ++_handles[_used].age;
    _spin_lock.Unlock();
  }
  u32 GetUsed() const { return _used; }
  bool IsValid(Handle<TYPE> h) {
    bool valid = (h.idx < COUNT) && (h.type == TYPE) && (_indices[h.idx] < _used) && (_handles[h.idx].age == h.age);
    return valid;
  }
  Handle<TYPE> GetHandleAt(u32 i) const { return _handles[i]; }
  
private:
  Handle<TYPE> _handles[COUNT];
  u32 _indices[COUNT];
  u32 _used;
  SpinLock _spin_lock;
};
