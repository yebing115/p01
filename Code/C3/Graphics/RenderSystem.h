#pragma once
#include "ECS/System.h"
#include "Model.h"
#include "Memory/HandleAlloc.h"
#include "Platform/PlatformConfig.h"

#define MODEL_COMPNENT 0x2000

class RenderSystem : public ISystem {
public:
  RenderSystem();
  ~RenderSystem();

  bool OwnComponentType(u16 type) override;
  bool CreateComponent(u16 type, COMPONENT_HANDLE& out_handle) override;
  bool DestroyComponent(COMPONENT_HANDLE handle) override;

  void SetModelFilename(COMPONENT_HANDLE handle, const String& filename);

private:
  HandleAlloc<C3_MAX_MODELS> _model_handles;
  array<Model, C3_MAX_MODELS> _models;
};
