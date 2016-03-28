#pragma once
#include "Data/DataType.h"
#include "Debug/C3Debug.h"
#include "Pattern/Handle.h"

template <u32 COUNT>
class HandleAlloc {
public:
  HandleAlloc(): _used(0) {
    for (u32 i = 0; i < COUNT; ++i) {
      auto& h = _handles[i];
      h.idx = i;
      h.age = 0;
      _indices[i] = i;
    }
  }
  void Clear() { _used = 0; }
  Handle Alloc() {
    c3_assert_return_x(_used < COUNT, Handle());
    return _handles[_used++];
  }
  void Free(Handle h) {
    //c3_assert(h.idx < COUNT);
    //c3_assert(_indices[h.idx] < _used);
    //c3_assert(_handles[h.idx].age == h.age);
    if (_indices[h.idx] != _used - 1) {
      u32 old_index = _indices[h.idx];
      swap(_indices[h.idx], _indices[_handles[_used - 1].idx]);
      swap(_handles[old_index], _handles[_used - 1]);
    }
    --_used;
    ++_handles[_used].age;
  }
  u32 GetUsed() const { return _used; }
  bool IsValid(Handle h) { 
    return (h.idx < COUNT) && (_indices[h.idx] < _used) && (_handles[h.idx].age == h.age);
  }
  Handle GetHandleAt(u32 i) const { return _handles[i]; }
  
  static HandleAlloc<COUNT>* Create() { return new HandleAlloc<COUNT>; }
  static void Destroy(HandleAlloc<COUNT>* ptr) { delete ptr; }
  
private:
  Handle _handles[COUNT];
  u32 _indices[COUNT];
  u32 _used;
};
