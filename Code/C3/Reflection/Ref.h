#pragma once
#include "Data/DataType.h"
#include "RefObject.h"

template <typename T>
class Ref : public RefBase {
public:
  friend void Swap(Ref<T>& first, Ref<T> second) { swap(first._ptr, second._ptr); }
  Ref(T* object = nullptr): _ptr(nullptr) { operator = (object); }
  Ref(const Ref<T>& source): _ptr(nullptr) { operator = (source); }
  ~Ref() { operator = (nullptr); }
  operator T* () const { return _ptr; }
  T* operator -> () const { return _ptr; }
  Ref<T>& operator = (T* new_object) {
    if (new_object) Increment(new_object);

    if (_ptr) {
      Decrement(_ptr);
      if (Unreferenced(_ptr)) delete static_cast<RefObject*>(_ptr);
    }

    _ptr = new_object;
    return *this;
  }
  Ref<T>& operator = (const Ref<T>& source) { return operator = (source._ptr); }
  T* GetPtr() const { return _ptr; }
private:
  T* _ptr;
};

inline bool RefBase::Unreferenced(RefObject* object) { return object->_ref_count == 0; }
inline void RefBase::Increment(RefObject* object) { ++object->_ref_count; }
inline void RefBase::Decrement(RefObject* object) { --object->_ref_count; }

