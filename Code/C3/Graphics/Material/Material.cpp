#include "C3PCH.h"
#include "Material.h"

void Material::Apply() {
  auto GR = GraphicsRenderer::Instance();
  Texture* texture = nullptr;
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
      GR->SetTexture(p->_tex2d._unit, texture->_handle, p->_tex2d._flags);
      break;
    default:
      ;
    }
  }
}
