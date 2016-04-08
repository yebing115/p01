#include "C3PCH.h"
#include "RenderSystem.h"

RenderSystem::RenderSystem() {
  auto GR = GraphicsRenderer::Instance();

  auto vsh_file = FileSystem::Instance()->OpenRead("Shaders/model.vsb");
  c3_assert(vsh_file && vsh_file->IsValid());
  auto vsh_mem = mem_alloc(vsh_file->GetSize());
  vsh_file->ReadBytes(vsh_mem->data, vsh_mem->size);
  FileSystem::Instance()->Close(vsh_file);
  auto vsh = GR->CreateShader(vsh_mem);

  auto fsh_file = FileSystem::Instance()->OpenRead("Shaders/model.fsb");
  c3_assert(fsh_file && fsh_file->IsValid());
  auto fsh_mem = mem_alloc(fsh_file->GetSize());
  fsh_file->ReadBytes(fsh_mem->data, fsh_mem->size);
  FileSystem::Instance()->Close(fsh_file);
  auto fsh = GR->CreateShader(fsh_mem);

  _program = GR->CreateProgram(vsh, fsh);
  c3_assert(_program);
}

RenderSystem::~RenderSystem() {
  GraphicsRenderer::Instance()->DestroyProgram(_program);
}

bool RenderSystem::OwnComponentType(HandleType type) const {
  return (type == MODEL_RENDERER_HANDLE);
}

GenericHandle RenderSystem::CreateComponent(EntityHandle entity, HandleType type) {
  if (type == MODEL_RENDERER_HANDLE) return CreateModelRenderer(entity);
  else return GenericHandle();
}

ModelRendererHandle RenderSystem::CreateModelRenderer(EntityHandle entity) {
  auto h = _model_renderer_handles.Alloc();
  if (h) {
    _model_renderer_map.insert(make_pair(entity, h));
    _model_renderer[h.idx]._entity = entity;
    _model_renderer[h.idx].Init();
  }
  return h;
}

void RenderSystem::DestroyModelRenderer(ModelRendererHandle handle) {
  if (!_model_renderer_handles.IsValid(handle)) return;
  _model_renderer_map.erase(_model_renderer[handle.idx]._entity);
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

void RenderSystem::Render(float dt, bool paused) {
  auto world = GameWorld::Instance();
  if (world->_camera_handles.GetUsed() == 0) return;
  auto GR = GraphicsRenderer::Instance();
  auto& camera = world->_cameras[world->_camera_handles.GetHandleAt(0).idx];
  auto view = GR->PushView();
  auto win_size = GR->GetWindowSize();
  camera.SetVerticalFovAndAspectRatio(DegToRad(20.f), win_size.x / win_size.y);
  GR->SetViewRect(view, 0, 0, win_size.x, win_size.y);
  GR->SetViewClear(view, C3_CLEAR_COLOR | C3_CLEAR_DEPTH, 0, 1.f);
  GR->SetViewTransform(view, camera.GetViewMatrix().ptr(), camera.GetProjectionMatrix().ptr());
  auto n = _model_renderer_handles.GetUsed();
  for (int i = 0; i < n; ++i) {
    auto& mr = _model_renderer[_model_renderer_handles.GetHandleAt(i).idx];
    if (!mr._model || mr._model->_state != ASSET_STATE_READY) continue;
    auto transform_handle = cast_to<TRANSFORM_HANDLE>(mr._entity, world->_transform_handles, world->_transform_map);
    if (transform_handle) {
      auto& transform = world->_transforms[transform_handle.idx];
      float4x4 m = float4x4::FromTRS(transform._position, transform._rotation, transform._scale * 0.1f);
      SpinLockGuard lock_guard(&mr._model->_lock);
      if (mr._model->_state != ASSET_STATE_READY) continue;
      auto model = (Model*)mr._model->_header->GetData();
      for (auto part = model->_parts; part < model->_parts + model->_num_parts; ++part) {
        GR->SetTransform(&m);
        GR->SetVertexBuffer(model->_vb);
        GR->SetIndexBuffer(model->_ib, part->_start_index, part->_num_indices);
        GR->SetState(C3_STATE_RGB_WRITE | C3_STATE_ALPHA_WRITE | C3_STATE_DEPTH_WRITE |
                     C3_STATE_CULL_CW | C3_STATE_DEPTH_TEST_LESS);
        float dist = camera._frustum.Distance(m.TransformPos(part->_aabb.CenterPoint()));
        GR->Submit(view, _program, depth_to_bits(dist));
      }
    }
  }
}
