#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Graphics/GraphicsTypes.h"
#include "Asset/AssetManager.h"

enum MaterialParamType {
  MATERIAL_PARAM_FLOAT,
  MATERIAL_PARAM_VEC2,
  MATERIAL_PARAM_VEC3,
  MATERIAL_PARAM_VEC4,
  MATERIAL_PARAM_TEXTURE2D,
  NUM_MATERIAL_PARAM_TYPES
};

struct MaterialParam {
  MaterialParamType _type;
  union {
    float _vec[4];
    struct {
      Asset* _asset;
      u32 _flags;
    } _tex2d;
  };
};

struct Material {
  Asset* _shader;
  vector<MaterialParam> _params;
};
