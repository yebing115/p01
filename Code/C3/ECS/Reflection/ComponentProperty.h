#pragma once
#include "ECS/Component.h"
#include "ECS/World.h"
#include "Reflection/Variant.h"

class IComponentProperty {
public:
  virtual Variant GetValue(COMPONENT_HANDLE) = 0;
  virtual void SetValue(COMPONENT_HANDLE, const Variant&) {}

  const char* _name;
};

template <typename S>
class ComponentPropertyInt : public IComponentProperty {
  typedef int (S::*Getter)(COMPONENT_HANDLE) const;
  typedef void (S::*Setter)(COMPONENT_HANDLE, int);
public:
  ComponentPropertyInt(const char* name, Getter getter, Setter setter = nullptr)
  : _getter(getter), _setter(setter) { _name = name; }
  Variant GetValue(COMPONENT_HANDLE cmp) override {
    auto sys = World::Instance()->GetSystem(cmp.type);
    if (!sys) return Variant();
    return ((static_cast<S*>(sys))->*_getter)(cmp);
  }
  void SetValue(COMPONENT_HANDLE cmp, const Variant& v) {
    if (!_setter) return;
    auto sys = World::Instance()->GetSystem(cmp.type);
    if (sys) ((static_cast<S*>(sys))->*_setter)(cmp, v.ToInt());
  }
private:
  Getter _getter;
  Setter _setter;
};
