#include "C3PCH.h"
#include "RenderSystem.h"

bool RenderSystem::OwnComponentType(HandleType type) {
  return (type == MODEL_RENDERER_HANDLE);
}

GenericHandle RenderSystem::CreateComponent(EntityHandle entity, HandleType type) {
  if (type == MODEL_RENDERER_HANDLE) return CreateModelRenderer(entity);
  else return GenericHandle();
}

ModelRendererHandle RenderSystem::CreateModelRenderer(EntityHandle entity) {
  auto h = _model_renderer_handles.Alloc();
  if (h) {
    _model_renderer[h.idx]._entity = entity;
    _model_renderer[h.idx].Init();
  }
  return h;
}

void RenderSystem::DestroyModelRenderer(ModelRendererHandle handle) {
  if (!_model_renderer_handles.IsValid(handle)) return;
  //auto& c = _model_renderer[handle.idx];
}

const char* RenderSystem::GetModelFilename(GenericHandle gh) const {
  auto h = cast_to<MODEL_RENDERER_HANDLE>(gh,
                                          _model_renderer_handles,
                                          _model_renderer_map);
  if (!h) return "";
  return _model_renderer[h.idx]._model->_desc._filename;
}

void RenderSystem::SetModelFilename(GenericHandle gh, const char* filename) {
  auto h = cast_to<MODEL_RENDERER_HANDLE>(gh,
                                          _model_renderer_handles,
                                          _model_renderer_map);
  if (!h) return;
  auto& m = _model_renderer[h.idx];;
  if (m._model) {
    if (strcmp(m._model->_desc._filename, filename) == 0) return;
    AssetManager::Instance()->Unload(m._model);
    m._part_list.clear();
  }
  m._model = AssetManager::Instance()->Load(ASSET_TYPE_MODEL, filename);
}

void RenderSystem::RenderModels() {

}
