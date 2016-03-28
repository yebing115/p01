#pragma once

#include "Data/DataType.h"

class IReflectProperty;
class IReflectMethod;
class ReflectInfo {
public:
  const char* _name;
  ReflectInfo* _parent;
  vector<IReflectProperty*> _properties;
  vector<IReflectMethod*> _methods;
};
