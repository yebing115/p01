#pragma once

#include "Data/DataType.h"
#include "Data/String.h"
#include "ECS/Component.h"

class Variant;
typedef vector<Variant> VariantList;
class Variant {
public:
  enum Type {
    T_VOID,
    T_BOOL,
    T_INT,
    T_FLOAT,
    T_STRING,
    T_COMPONENT,
  };
  Variant() : _type(T_VOID) {}
  Variant(int i) : _type(T_INT), _i(i) {}
  Variant(float f) : _type(T_FLOAT), _f(f) {}
  Variant(COMPONENT_HANDLE h) : _type(T_COMPONENT), _component(h) {}
  Variant(const Variant&) {}
  Variant(Variant&&) {}
  ~Variant() {}
  int ToInt() const { return _i; }
  template <typename T> T To() const { return T(); }
private:
  Type _type;
  union {
    bool _b;
    int _i;
    float _f;
    float2 _v2;
    float3 _v3;
    float4 _v4;
    float3x3 _m3;
    float4x4 _m4;
    String _str;
    COMPONENT_HANDLE _component;
  };
};
