#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Graphics/GraphicsTypes.h"
#include "Asset/AssetManager.h"

#define MAX_MATERIAL_TECHNIQUE_NAME_LEN 64
#define MAX_MATERIAL_PASS_NAME_LEN 64
#define MAX_MATERIAL_KEY_LEN 64
#define MAX_MATERIAL_PARAMS 8
#define MAX_MATERIAL_SUB_SHADERS 8

enum MaterialParamType {
  MATERIAL_PARAM_FLOAT,
  MATERIAL_PARAM_VEC2,
  MATERIAL_PARAM_VEC3,
  MATERIAL_PARAM_VEC4,
  MATERIAL_PARAM_TEXTURE2D,
  NUM_MATERIAL_PARAM_TYPES
};

struct MaterialParam {
  char _name[MAX_MATERIAL_KEY_LEN];
  ConstantHandle _constant_handle;
  MaterialParamType _type;
  union {
    float _vec[4];
    struct {
      Asset* _asset;
      u32 _flags;
      u8 _unit;
    } _tex2d;
  };
};

struct SubShader {
  char _technique[MAX_MATERIAL_TECHNIQUE_NAME_LEN];
  char _pass[MAX_MATERIAL_PASS_NAME_LEN];
  ProgramHandle _program;
  u32 _num_params;
  MaterialParam _params[MAX_MATERIAL_PARAMS];
};

struct MaterialShader {
  u32 _num_sub_shaders;
  SubShader _sub_shaders[MAX_MATERIAL_SUB_SHADERS];
};

struct Material {
  Asset* _shader_asset;
  u32 _num_params;
  MaterialParam _params[MAX_MATERIAL_PARAMS];

  ProgramHandle Apply(const char* tech, const char* pass);
  void ApplyParams(SubShader* sub_shader);
};
