#pragma once

#include "Variant.h"

class Object;
class IReflectProperty {
public:
  const char* _name;

  virtual Variant GetValue(Object*) const = 0;
  virtual void SetValue(Object*, const Variant&) {}
};

template <typename O>
class ReflectPropertyInt : public IReflectProperty {
  typedef int O::*Ptr;
  typedef int (O::*Getter)() const;
  typedef void (O::*Setter)(int);
public:
  ReflectPropertyInt(const char* name, Ptr ptr)
  : _ptr(ptr), _getter(nullptr), _setter(nullptr) { _name = name; }
  ReflectPropertyInt(const char* name, Getter getter, Setter setter = nullptr)
  : _ptr(nullptr), _getter(getter), _setter(setter) { _name = name; }
  Variant GetValue(Object* obj) const override {
    if (_ptr) return (static_cast<O*>(obj))->*_ptr;
    else if (_getter) return ((static_cast<O*>(obj))->*_getter)();
    else return Variant();
  }
  void SetValue(Object* obj, const Variant& value) override {
    if (_ptr) (static_cast<O*>(obj))->*_ptr = value.ToInt();
    else if (_setter) return ((static_cast<O*>(obj))->*_setter)(value.ToInt());
  }
private:
  Ptr _ptr;
  Getter _getter;
  Setter _setter;
};
