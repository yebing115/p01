#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Graphics/GraphicsTypes.h"
#include "Asset/AssetManager.h"

#define MAX_MATERIAL_TECHNIQUE_NAME_LEN 64
#define MAX_MATERIAL_PASS_NAME_LEN 64
#define MAX_MATERIAL_KEY_LEN 64

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

struct MaterialShader {
  char _technique[MAX_MATERIAL_TECHNIQUE_NAME_LEN];
  char _pass[MAX_MATERIAL_PASS_NAME_LEN];
  ProgramHandle _program;
  u32 _num_params;
  MaterialParam _params[];

  static size_t ComputeSize(u16 num_params) {
    return sizeof(MaterialShader) + num_params * sizeof(MaterialParam);
  }
};

struct Material {
  Asset* _material_shader;
  u32 _num_params;
  MaterialParam _params[];

  static size_t ComputeSize(u16 num_params) {
    return sizeof(Material) + num_params * sizeof(MaterialParam);
  }
};
