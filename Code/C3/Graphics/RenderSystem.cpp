#include "RenderSystem.h"

RenderSystem::RenderSystem() {}

RenderSystem::~RenderSystem() {}

bool RenderSystem::OwnComponentType(u16 type) {
  if (type == MODEL_COMPNENT) return true;
  else return false;
}

bool RenderSystem::CreateComponent(u16 type, COMPONENT_HANDLE& out_handle) {
  out_handle = _model_handles.Alloc();
  return (bool)out_handle;
}

bool RenderSystem::DestroyComponent(COMPONENT_HANDLE handle) {
  if (!_model_handles.IsValid(handle)) return false;
  _model_handles.Free(handle);
  return true;
}

void RenderSystem::SetModelFilename(COMPONENT_HANDLE handle, const String& filename) {
  if (!_model_handles.IsValid(handle)) return;
  auto& model = _models[handle.idx];
  model._filename = filename;
}
