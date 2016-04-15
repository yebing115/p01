#include "C3PCH.h"
#include "RenderSystem.h"

RenderSystem::RenderSystem() {
  auto GR = GraphicsRenderer::Instance();
  _constant_light_type = GR->CreateConstant(String::GetID("light_type"), CONSTANT_INT);
  _constant_light_color = GR->CreateConstant(String::GetID("light_color"), CONSTANT_VEC3);
  _constant_light_pos = GR->CreateConstant(String::GetID("light_pos"), CONSTANT_VEC3);
  _constant_light_dir = GR->CreateConstant(String::GetID("light_dir"), CONSTANT_VEC3);
  _constant_light_falloff = GR->CreateConstant(String::GetID("light_falloff"), CONSTANT_VEC4);
  _constant_light_transform = GR->CreateConstant(String::GetID("light_transform"), CONSTANT_MAT4);
  TextureHandle th = GR->CreateTexture2D(1024, 1024, 1, DEPTH_32_FLOAT_TEXTURE_FORMAT,
                                         C3_TEXTURE_RT);
  _shadow_fb = GR->CreateFrameBuffer(1, &th);
}

RenderSystem::~RenderSystem() {
  auto GR = GraphicsRenderer::Instance();
  GR->DestroyConstant(_constant_light_type);
  GR->DestroyConstant(_constant_light_color);
  GR->DestroyConstant(_constant_light_pos);
  GR->DestroyConstant(_constant_light_dir);
  GR->DestroyConstant(_constant_light_falloff);
  GR->DestroyConstant(_constant_light_transform);
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

LightHandle RenderSystem::CreateLight(EntityHandle e) {
  auto h = _light_handles.Alloc();
  if (h) {
    _light_entity_map.insert(make_pair(e, h));
    _model_renderer[h.idx]._entity = e;
    _model_renderer[h.idx].Init();
  }
  return h;
}

void RenderSystem::DestroyLight(LightHandle h) {
  if (!_light_handles.IsValid(h)) return;
  _light_entity_map.erase(_lights[h.idx]._entity);
}

void RenderSystem::SetLightType(GenericHandle gh, LightType type) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._type = type;
}

void RenderSystem::SetLightColor(GenericHandle gh, const Color& color) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._color = color;
}

void RenderSystem::SetLightIntensity(GenericHandle gh, float intensity) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._intensity = intensity;
}

void RenderSystem::SetLightCastShadow(GenericHandle gh, bool shadow) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._cast_shadow = shadow;
}

void RenderSystem::SetLightDir(GenericHandle gh, const float3& dir) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._dir = dir;
}

void RenderSystem::SetLightPos(GenericHandle gh, const float3& pos) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._pos = pos;
}

void RenderSystem::SetLightDistFalloff(GenericHandle gh, const float2& fallof) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._dist_falloff = fallof;
}

void RenderSystem::SetLightAngleFalloff(GenericHandle gh, const float2& fallof) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return;
  _lights[h.idx]._angle_falloff = fallof;
}

LightType RenderSystem::GetLightType(GenericHandle gh) const {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return DIRECTIONAL_LIGHT;
  return _lights[h.idx]._type;
}

Color RenderSystem::GetLightColor(GenericHandle gh) const {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return Color::NO_COLOR;
  return _lights[h.idx]._color;
}

float RenderSystem::GetLightIntensity(GenericHandle gh) const {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return 0.f;
  return _lights[h.idx]._intensity;
}

bool RenderSystem::GetLightCastShadow(GenericHandle gh) const {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return false;
  return _lights[h.idx]._cast_shadow;
}

float3 RenderSystem::GetLightDir(GenericHandle gh) const {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return float3::unitY;
  return _lights[h.idx]._dir;
}

float3 RenderSystem::GetLightPos(GenericHandle gh, const float3& pos) const {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return float3::zero;
  return _lights[h.idx]._pos;
}

float2 RenderSystem::GetLightDistFalloff(GenericHandle gh) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return float2::zero;
  return _lights[h.idx]._dist_falloff;
}

float2 RenderSystem::GetLightAngleFalloff(GenericHandle gh) {
  auto h = cast_to<LIGHT_HANDLE>(gh, _light_handles, _light_entity_map);
  if (!h) return float2::zero;
  return _lights[h.idx]._angle_falloff;
}

void RenderSystem::GetLights(const LightHandle*& out_handles, u32& out_num_handles, Light*& out_lights) {
  out_handles = _light_handles.GetPointer();
  out_num_handles = _light_handles.GetUsed();
  out_lights = _lights;
}

void RenderSystem::Render(float dt, bool paused) {
  Light sun_light;
  sun_light.Init();
  sun_light._type = DIRECTIONAL_LIGHT;
  sun_light._color = Color(1.0f, 0.68f, 0.41f);
  sun_light._intensity = 1.f;
  sun_light._dir.Set(0.1294095, -0.9659258, 0.2241439);

  auto world = GameWorld::Instance();
  if (world->_camera_handles.GetUsed() == 0) return;
  auto GR = GraphicsRenderer::Instance();
  auto& camera = world->_cameras[world->_camera_handles.GetHandleAt(0).idx];
  u8 view;
  auto win_size = GR->GetWindowSize();
  camera.SetAspect(win_size.x / win_size.y);
  camera.SetClipPlane(1, 4000);
  auto camera_volume = camera._frustum.ToPBVolume();
  auto n = _model_renderer_handles.GetUsed();

  view = GR->PushView();
  GR->SetViewFrameBuffer(view, _shadow_fb);
  GR->SetViewRect(view, 0, 0, 1024, 1024);
  GR->SetViewClear(view, C3_CLEAR_DEPTH, 0, 1.f);
  float4x4 light_view, light_proj;
  GetLightViewProj(&sun_light, light_view, light_proj);
  GR->SetViewTransform(view, light_view.ptr(), light_proj.ptr());
  for (int i = 0; i < n; ++i) {
    auto& mr = _model_renderer[_model_renderer_handles.GetHandleAt(i).idx];
    if (!mr._model || mr._model->_state != ASSET_STATE_READY) continue;
    auto transform_handle = cast_to<TRANSFORM_HANDLE>(mr._entity, world->_transform_handles, world->_transform_map);
    if (transform_handle) {
      auto& transform = world->_transforms[transform_handle.idx];
      float4x4 m = float4x4::FromTRS(transform._position, transform._rotation, transform._scale);
      SpinLockGuard lock_guard(&mr._model->_lock);
      if (mr._model->_state != ASSET_STATE_READY) continue;
      auto model = (Model*)mr._model->_header->GetData();
      auto b = model->_aabb;
      for (auto part = model->_parts; part < model->_parts + model->_num_parts; ++part) {
        if (camera_volume.InsideOrIntersects(part->_aabb.Transform(m).MinimalEnclosingAABB()) == TestOutside) continue;
        GR->SetTransform(&m);
        GR->SetVertexBuffer(model->_vb);
        GR->SetIndexBuffer(model->_ib, part->_start_index, part->_num_indices);
        GR->SetState(C3_STATE_DEPTH_WRITE | C3_STATE_DEPTH_TEST_LESS | C3_STATE_CULL_CW);
        float dist = camera._frustum.Distance(m.TransformPos(part->_aabb.CenterPoint()));
        auto material = (Material*)model->_materials[part->_material_index]->_header->GetData();
        auto program = material->Apply("Forward", "Shadow");
        GR->Submit(view, program, depth_to_bits(dist));
      }
    }
  }

  view = GR->PushView();
  GR->SetViewRect(view, 0, 0, win_size.x, win_size.y);
  GR->SetViewClear(view, C3_CLEAR_COLOR | C3_CLEAR_DEPTH, 0, 1.f);
  GR->SetViewTransform(view, camera.GetViewMatrix().ptr(), camera.GetProjectionMatrix().ptr());
  for (int i = 0; i < n; ++i) {
    auto& mr = _model_renderer[_model_renderer_handles.GetHandleAt(i).idx];
    if (!mr._model || mr._model->_state != ASSET_STATE_READY) continue;
    auto transform_handle = cast_to<TRANSFORM_HANDLE>(mr._entity, world->_transform_handles, world->_transform_map);
    if (transform_handle) {
      auto& transform = world->_transforms[transform_handle.idx];
      float4x4 m = float4x4::FromTRS(transform._position, transform._rotation, transform._scale);
      SpinLockGuard lock_guard(&mr._model->_lock);
      if (mr._model->_state != ASSET_STATE_READY) continue;
      auto model = (Model*)mr._model->_header->GetData();
      for (auto part = model->_parts; part < model->_parts + model->_num_parts; ++part) {
        //if (camera_volume.InsideOrIntersects(part->_aabb.Transform(m).MinimalEnclosingAABB()) == TestOutside) continue;
        ApplyLight(&sun_light);
        GR->SetTransform(&m);
        GR->SetVertexBuffer(model->_vb);
        GR->SetIndexBuffer(model->_ib, part->_start_index, part->_num_indices);
        GR->SetTexture(15, _shadow_fb, 0, C3_TEXTURE_COMPARE_LESS);
        GR->SetState(C3_STATE_RGB_WRITE | C3_STATE_ALPHA_WRITE | C3_STATE_DEPTH_WRITE |
                     C3_STATE_CULL_CW | C3_STATE_DEPTH_TEST_LEQUAL);
        float dist = camera._frustum.Distance(m.TransformPos(part->_aabb.CenterPoint()));
        auto material = (Material*)model->_materials[part->_material_index]->_header->GetData();
        auto program = material->Apply("Forward", "Geometry");
        GR->Submit(view, program, depth_to_bits(dist));
      }
    }
  }
}

void RenderSystem::ApplyLight(Light* light) {
  int type = light->_type;
  float3 color = *(const float3*)&light->_color * light->_intensity;
  float4 falloff(light->_dist_falloff.x, light->_dist_falloff.y,
                 Cos(light->_angle_falloff.x), Cos(light->_angle_falloff.y));
  auto GR = GraphicsRenderer::Instance();
  GR->SetConstant(_constant_light_type, &type);
  GR->SetConstant(_constant_light_color, &color);
  GR->SetConstant(_constant_light_pos, &light->_pos);
  GR->SetConstant(_constant_light_dir, &light->_dir);
  GR->SetConstant(_constant_light_falloff, &falloff);
  
  Frustum light_frustum;
  light_frustum.SetKind(FrustumSpaceD3D, FrustumRightHanded);
  light_frustum.SetFrame(float3(0, 2000, 0), light->_dir, light->_dir.Perpendicular());
  light_frustum.SetOrthographic(4000.f, 4000.f);
  light_frustum.SetViewPlaneDistances(0.f, 2000.f);
  float4x4 m = float4x4::Translate(0.5f, 0.5f, 0.f) * float4x4::Scale(0.5f, -0.5f, 1.f) * light_frustum.ComputeViewProjMatrix();
  auto kk = m.TransformPos(float3::zero);
  GR->SetConstant(_constant_light_transform, &m);
}

void RenderSystem::GetLightViewProj(Light* light, float4x4& view, float4x4& proj) const {
  Frustum light_frustum;
  light_frustum.SetKind(FrustumSpaceD3D, FrustumRightHanded);
  light_frustum.SetFrame(float3(0, 2000, 0), light->_dir, light->_dir.Perpendicular());
  light_frustum.SetOrthographic(4000.f, 4000.f);
  light_frustum.SetViewPlaneDistances(0.f, 2000.f);
  view = light_frustum.ComputeViewMatrix();
  proj = light_frustum.ComputeProjectionMatrix();
}
