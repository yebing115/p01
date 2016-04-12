#include "C3PCH.h"
#include "Material.h"

void Material::Apply() {
  auto GR = GraphicsRenderer::Instance();
  Texture* texture = nullptr;
  u32 flags;
  for (auto p = _params; p < _params + _num_params; ++p) {
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
