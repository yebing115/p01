#pragma once

#include "RefObject.h"
#include "ReflectInfo.h"

#define SUPPORT_REFLECT(ClassName) \
public: \
  virtual ReflectInfo* GetReflectInfoV() { return s_ri; } \
  static ReflectInfo* GetReflectInfoS() { return s_ri; } \
  static void BindReflectInfo(ReflectInfo* info) { s_ri = info; } \
private: \
  static ReflectInfo* s_ri;

#define IMPLEMENT_REFLECT(ClassName) \
  ReflectInfo* ClassName::s_ri = nullptr;

class ReflectInfo;
class Object : public RefObject {
  SUPPORT_REFLECT(Object)
};
