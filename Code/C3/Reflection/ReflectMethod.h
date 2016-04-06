#pragma once

#include "Variant.h"
#include <type_traits>

class Object;
class IReflectMethod {
public:
  virtual Variant Invoke(Object* obj, const VariantList& args) = 0;

  const char* _name;
};

template <typename O, typename Ret, typename... Param>
class ReflectMethod : public IReflectMethod {
  typedef Ret(O::*Func)(Param...);
  typedef void(O::*VoidFunc)(Param...);
public:
  ReflectMethod(const char* name, Func f) : _func(f), _void_func(nullptr) { _name = name; }
  ReflectMethod(const char* name, VoidFunc f) : _func(nullptr), _void_func(f) { _name = name; }
  Variant Invoke(Object* obj, const VariantList& args) override {
    if (args.size() != sizeof...(Param)) return Variant();
    if (_func) return InvokeFunc(static_cast<O*>(obj), args, std::index_sequence_for<Param...>());
    else {
      if (_void_func) InvokeVoidFunc(static_cast<O*>(obj), args, std::index_sequence_for<Param...>());
      return Variant();
    }
  }
private:
  template <size_t... I>
  Ret InvokeFunc(O* obj, const VariantList& args, std::index_sequence<I...>) {
    return (obj->*_func)(args[I].To<Param>()...);
  }
  template <size_t... I>
  void InvokeVoidFunc(O* obj, const VariantList& args, std::index_sequence<I...>) {
    (obj->*_func)(args[I].To<Param>()...);
  }

  Func _func;
  VoidFunc _void_func;
};

template <typename O, typename Ret, typename... Param>
class ReflectComponentMethod : public IReflectMethod {
  typedef Ret(O::*Func)(Param...);
  typedef void(O::*VoidFunc)(Param...);
public:
  ReflectMethod(const char* name, Func f) : _func(f), _void_func(nullptr) { _name = name; }
  ReflectMethod(const char* name, VoidFunc f) : _func(nullptr), _void_func(f) { _name = name; }
  Variant Invoke(Object* obj, const VariantList& args) override {
    if (args.size() != sizeof...(Param)) return Variant();
    if (_func) return InvokeFunc(static_cast<O*>(obj), args, std::index_sequence_for<Param...>());
    else {
      if (_void_func) InvokeVoidFunc(static_cast<O*>(obj), args, std::index_sequence_for<Param...>());
      return Variant();
    }
  }
private:
  template <size_t... I>
  Ret InvokeFunc(O* obj, const VariantList& args, std::index_sequence<I...>) {
    return (obj->*_func)(args[I].To<Param>()...);
  }
  template <size_t... I>
  void InvokeVoidFunc(O* obj, const VariantList& args, std::index_sequence<I...>) {
    (obj->*_func)(args[I].To<Param>()...);
  }

  Func _func;
  VoidFunc _void_func;
};