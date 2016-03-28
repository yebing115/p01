#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Reflection/Variant.h"

class Shader;
class Material {
public:
  Material(const String& fname);
  ~Material();

private:
  String _filename;
  Shader* _shader;
  struct Param {
    String name;
    Variant value;
  };
  vector<Param> _params;
};
