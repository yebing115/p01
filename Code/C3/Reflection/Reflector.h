#pragma once

#include "Data/DataType.h"
#include "Data/String.h"
#include "MetaData.h"
#include "ReflectMethod.h"
#include "ReflectProperty.h"
#include "ReflectInfo.h"

class MetaData;
class Reflector {
public:
  Reflector(const char* name, ReflectInfo* parent = nullptr) {
    _info = new ReflectInfo();
    _info->_name = name;
    _info->_parent = parent;
  }
  ~Reflector() {
    if (_info) delete _info;
  }
  template <typename O>
  void Reflect(const char* name, int O::* field, const MetaData* meta = nullptr) {
    (void)meta;
    _info->_properties.push_back(new ReflectPropertyInt<O>(name, field));
  }
  template <typename O, typename R, typename... Params>
  void Reflect(const char* name, R(O::*func)(Params...), const MetaData* meta = nullptr) {
    (void)meta;
    _info->_methods.push_back(new ReflectMethod<O, R, Params...>(name, func));
  }
  template <typename O, typename... Params>
  void Reflect(const char* name, void(O::*func)(Params...), const MetaData* meta = nullptr) {
    (void)meta;
    _info->_methods.push_back(new ReflectMethod<O, Params...>(name, func));
  }
  ReflectInfo* GetReflectInfo() {
    auto info = _info;
    _info = nullptr;
    return _info;
  }
private:
  ReflectInfo* _info;
};
