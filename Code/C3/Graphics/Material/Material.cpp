#include "C3PCH.h"
#include "Material.h"

ProgramHandle Material::Apply(const char* tech, const char* pass) {
  if (!_shader_asset) return ProgramHandle();
  SpinLockGuard lock_guard(&_shader_asset->_lock);
  if (_shader_asset->_state != ASSET_STATE_READY) return ProgramHandle();
  auto shader = (MaterialShader*)_shader_asset->_header->GetData();
  for (u32 i = 0; i < shader->_num_sub_shaders; ++i) {
    auto& sub_shader = shader->_sub_shaders[i];
    if (strcmp(sub_shader._technique, tech) == 0 && strcmp(sub_shader._pass, pass) == 0) {
      ApplyParams(&sub_shader);
      return sub_shader._program;
    }
  }
  return ProgramHandle();
}

void Material::ApplyParams(SubShader* sub_shader) {
  auto GR = GraphicsRenderer::Instance();
  Texture* texture = nullptr;
  u32 flags;
  for (u32 i = 0; i < sub_shader->_num_params; ++i) {
    // check material override
    MaterialParam* p = sub_shader->_params + i;
    for (u32 j = 0; j < _num_params; ++j) {
      if (strcmp(_params[j]._name, p->_name) == 0) {
        p = _params + j;
        break;
      }
    }
    
    switch (p->_type) {
    case MATERIAL_PARAM_FLOAT:
    case MATERIAL_PARAM_VEC2:
    case MATERIAL_PARAM_VEC3:
    case MATERIAL_PARAM_VEC4:
      GR->SetConstant(p->_constant_handle, p->_vec);
      break;
    case MATERIAL_PARAM_TEXTURE2D:
      texture = (Texture*)p->_tex2d._asset->_header->GetData();
      flags = p->_tex2d._flags;
      // TODO: dirty workaround.
      if (strstr(p->_name, "normal") == nullptr) flags |= C3_TEXTURE_SRGB;
      GR->SetTexture(p->_tex2d._unit, texture->_handle, flags);
      break;
    default:
      ;
    }
  }
}
