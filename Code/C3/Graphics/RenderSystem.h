#pragma once

#include "Data/DataType.h"
#include "ECS/System.h"
#include "Graphics/Model/ModelRenderer.h"

class RenderSystem : public ISystem {
public:
  RenderSystem();
  ~RenderSystem();

  bool OwnComponentType(HandleType type) const override;
  GenericHandle CreateComponent(EntityHandle entity, HandleType type) override;

  ModelRendererHandle CreateModelRenderer(EntityHandle entity);
  void DestroyModelRenderer(ModelRendererHandle handle);
  const char* GetModelFilename(GenericHandle gh) const;
  void SetModelFilename(GenericHandle gh, const char* filename);

  void Render(float dt, bool paused) override;

private:
  ModelRenderer _model_renderer[C3_MAX_MODEL_RENDERERS];
  HandleAlloc<MODEL_RENDERER_HANDLE, C3_MAX_MODEL_RENDERERS> _model_renderer_handles;
  unordered_map<EntityHandle, ModelRendererHandle> _model_renderer_map;
};
