#pragma once
#include "Material.h"

#define MAX_MATERIAL_SHADER_NAME_LEN 64

struct MaterialShaderPart {
  char _technique[MAX_MATERIAL_SHADER_NAME_LEN];
  char _pass[MAX_MATERIAL_SHADER_NAME_LEN];
  u64 _render_state;
  vector<MaterialParam> _params;

  Asset* _program;
};

struct MaterialShader {
  vector<MaterialShaderPart> _parts;
};
