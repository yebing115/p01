#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "UniformMetaData.h"
#include "Pattern/Handle.h"

class ShaderInstance;
class Shader {
public:
  enum UniformType {
    UT_INVALID = -1,
    UT_BOOL,
    UT_FLOAT,
    UT_FLOAT2,
    UT_FLOAT3,
    UT_FLOAT4,
    UT_FLOAT2X2,
    UT_FLOAT3X3,
    UT_FLOAT4X4,
    UT_TIME,
    UT_COLOR,
    UT_SAMPLER,
  };
  struct Uniform {
    String _name;
    UniformType _type;
    String _desc;
    UniformMetaData* _meta;
  };
  Shader(const String& fname);
  ~Shader();
private:
  String _filename;
  String _vs;
  String _fs;
  struct Pass {
    String _name;
    StringList _vs_defines;
    StringList _fs_defines;
    ShaderInstance* _shader_inst;
  };
  vector<Pass> _passes;
  vector<Uniform> _uniforms;
};

class ShaderInstance {
public:
  Handle _program;
  Shader* _shader;
  String _name;
  StringList _vs_defines;
  StringList _fs_defines;
};
