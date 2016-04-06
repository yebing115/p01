#pragma once
#include "ECS/GameWorld.h"
#include "Reflection/Variant.h"

class IComponentMethod {
public:
  virtual Variant Invoke(RAW_HANDLE_TYPE cmp, const VariantList& args) = 0;

  const char* _name;
};

template <typename S, HandleType TYPE, typename Ret, typename... Param>
class ComponentMethod : public IComponentMethod {
  typedef Ret(S::*Func)(Handle<TYPE>, Param...);
  typedef void(S::*VoidFunc)(Handle<TYPE>, Param...);
public:
  ComponentMethod(const char* name, Func f)
  : _func(f), _void_func(nullptr) { _name = name; }
  ComponentMethod(const char* name, VoidFunc f)
  : _func(nullptr), _void_func(f) { _name = name; }
  Variant Invoke(RAW_HANDLE_TYPE cmp, const VariantList& args) override {
    if (args.size() != sizeof...(Param)) return Variant();
    auto sys = GameWorld::Instance()->GetSystem(cmp.type);
    if (!sys) return Variant();
    if (_func) return InvokeFunc(static_cast<S*>(sys), Handle<TYPE>(cmp), args, std::index_sequence_for<Param...>());
    else {
      if (_void_func) InvokeVoidFunc(static_cast<S*>(sys), Handle<TYPE>(cmp), args, std::index_sequence_for<Param...>());
      return Variant();
    }
  }
private:
  template <size_t... I>
  Ret InvokeFunc(S* sys, Handle<TYPE> cmp, const VariantList& args, std::index_sequence<I...>) {
    return (sys->*_func)(cmp, args[I].To<Param>()...);
  }
  template <HandleType TYPE, size_t... I>
  void InvokeVoidFunc(S* sys, Handle<TYPE> cmp, const VariantList& args, std::index_sequence<I...>) {
    (sys->*_func)(cmp, args[I].To<Param>()...);
  }

  Func _func;
  VoidFunc _void_func;
};
