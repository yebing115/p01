#pragma once
#include "ECS/GameWorld.h"
#include "Reflection/Variant.h"

class IComponentProperty {
public:
  virtual Variant GetValue(RAW_HANDLE_TYPE) = 0;
  virtual void SetValue(RAW_HANDLE_TYPE, const Variant&) {}

  const char* _name;
};

template <HandleType TYPE, typename S>
class ComponentPropertyInt : public IComponentProperty {
  typedef int (S::*Getter)(Handle) const;
  typedef void (S::*Setter)(Handle, int);
public:
  ComponentPropertyInt(const char* name, Getter getter, Setter setter = nullptr)
  : _getter(getter), _setter(setter) { _name = name; }
  Variant GetValue(RAW_HANDLE_TYPE cmp) override {
    auto sys = GameWorld::Instance()->GetSystem(TYPE);
    if (!sys) return Variant();
    return ((static_cast<S*>(sys))->*_getter)(Handle<TYPE>(cmp));
  }
  void SetValue(RAW_HANDLE_TYPE cmp, const Variant& v) {
    if (!_setter) return;
    auto sys = GameWorld::Instance()->GetSystem(TYPE);
    if (sys) ((static_cast<S*>(sys))->*_setter)(Handle<TYPE>(cmp), v.ToInt());
  }
private:
  Getter _getter;
  Setter _setter;
};
