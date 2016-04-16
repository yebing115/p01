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
  TextureHandle th = GR->CreateTexture2D(4096, 4096, 1, DEPTH_32_FLOAT_TEXTURE_FORMAT,
                                         C3_TEXTURE_RT);
  _shadow_fb = GR->CreateFrameBuffer(1, &th);

  _num_lights = 0;
  _num_models = 0;
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

void RenderSystem::CreateComponent(EntityHandle entity, HandleType type) {
  if (type == MODEL_RENDERER_HANDLE) CreateModelRenderer(entity);
  else if (type == LIGHT_HANDLE) CreateLight(entity);
}

void RenderSystem::CreateModelRenderer(EntityHandle entity) {
  c3_assert_return(_num_models < C3_MAX_MODEL_RENDERERS);
  ModelRenderer* model = _models + _num_models;
  model->_entity = entity;
  model->Init();
  _model_map.insert(EntityMap::value_type(entity, _num_models));
  ++_num_models;
}

void RenderSystem::DestroyModelRenderer(EntityHandle e) {
  auto it = _model_map.find(e);
  if (it != _model_map.end()) {
    auto index = it->second;
    --_num_models;
    if (index != _num_models) {
      memcpy(_models + index, _models + _num_models, sizeof(ModelRenderer));
      auto moved_entity = _models[index]._entity;
      _model_map[moved_entity] = index;
    }
    _model_map.erase(e);
  }
}

const char* RenderSystem::GetModelFilename(EntityHandle e) const {
  auto model = FindModel(e);
  if (!model) return "";
  return model->_asset->_desc._filename;
}

void RenderSystem::SetModelFilename(EntityHandle e, const char* filename) {
  auto model = FindModel(e);
  if (!model) return;
  if (model->_asset) {
    if (strcmp(model->_asset->_desc._filename, filename) == 0) return;
    AssetManager::Instance()->Unload(model->_asset);
    model->_part_list.clear();
  }
  model->_asset = AssetManager::Instance()->Load(ASSET_TYPE_MODEL, filename);
}

ModelRenderer* RenderSystem::FindModel(EntityHandle e) const {
  auto it = _model_map.find(e);
  if (it != _model_map.end()) return (ModelRenderer*)_models + it->second;
  return nullptr;
}

void RenderSystem::CreateLight(EntityHandle e) {
  c3_assert_return(_num_models < C3_MAX_LIGHTS);
  Light* light = _lights + _num_lights;
  light->_entity = e;
  light->Init();
  _light_map.insert(EntityMap::value_type(e, _num_lights));
  ++_num_lights;
}

void RenderSystem::DestroyLight(EntityHandle e) {
  auto it = _light_map.find(e);
  if (it != _light_map.end()) {
    auto index = it->second;
    --_num_lights;
    if (index != _num_lights) {
      memcpy(_lights + index, _lights + _num_lights, sizeof(Light));
      auto moved_entity = _lights[index]._entity;
      _light_map[moved_entity] = index;
    }
    _light_map.erase(e);
  }
}

void RenderSystem::SetLightType(EntityHandle e, LightType type) {
  auto light = FindLight(e);
  if (!light) return;
  light->_type = type;
}

void RenderSystem::SetLightColor(EntityHandle e, const Color& color) {
  auto light = FindLight(e);
  if (!light) return;
  light->_color = color;
}

void RenderSystem::SetLightIntensity(EntityHandle e, float intensity) {
  auto light = FindLight(e);
  if (!light) return;
  light->_intensity = intensity;
}

void RenderSystem::SetLightCastShadow(EntityHandle e, bool shadow) {
  auto light = FindLight(e);
  if (!light) return;
  light->_cast_shadow = shadow;
}

void RenderSystem::SetLightDir(EntityHandle e, const float3& dir) {
  auto light = FindLight(e);
  if (!light) return;
  light->_dir = dir;
}

void RenderSystem::SetLightPos(EntityHandle e, const float3& pos) {
  auto light = FindLight(e);
  if (!light) return;
  light->_pos = pos;
}

void RenderSystem::SetLightDistFalloff(EntityHandle e, const float2& fallof) {
  auto light = FindLight(e);
  if (!light) return;
  light->_dist_falloff = fallof;
}

void RenderSystem::SetLightAngleFalloff(EntityHandle e, const float2& fallof) {
  auto light = FindLight(e);
  if (!light) return;
  light->_angle_falloff = fallof;
}

LightType RenderSystem::GetLightType(EntityHandle e) const {
  auto light = FindLight(e);
  if (!light) return DIRECTIONAL_LIGHT;
  return light->_type;
}

Color RenderSystem::GetLightColor(EntityHandle e) const {
  auto light = FindLight(e);
  if (!light) return Color::NO_COLOR;
  return light->_color;
}

float RenderSystem::GetLightIntensity(EntityHandle e) const {
  auto light = FindLight(e);
  if (!light) return 0.f;
  return light->_intensity;
}

bool RenderSystem::GetLightCastShadow(EntityHandle e) const {
  auto light = FindLight(e);
  if (!light) return false;
  return light->_cast_shadow;
}

float3 RenderSystem::GetLightDir(EntityHandle e) const {
  auto light = FindLight(e);
  if (!light) return -float3::unitY;
  return light->_dir;
}

float3 RenderSystem::GetLightPos(EntityHandle e, const float3& pos) const {
  auto light = FindLight(e);
  if (!light) return float3::zero;
  return light->_pos;
}

float2 RenderSystem::GetLightDistFalloff(EntityHandle e) {
  auto light = FindLight(e);
  if (!light) return float2::zero;
  return light->_dist_falloff;
}

float2 RenderSystem::GetLightAngleFalloff(EntityHandle e) {
  auto light = FindLight(e);
  if (!light) return float2::zero;
  return light->_angle_falloff;
}

Light* RenderSystem::FindLight(EntityHandle e) const {
  auto it = _light_map.find(e);
  return (it == _light_map.end()) ? nullptr : (Light*)_lights + it->second;
}

Light* RenderSystem::GetLights(int* num_lights) const {
  *num_lights = _num_lights;
  return (Light*)_lights;
}

void RenderSystem::Render(float dt, bool paused) {
  Light sun_light;
  sun_light.Init();
  sun_light._type = DIRECTIONAL_LIGHT;
  sun_light._color = Color(1.0f, 0.68f, 0.41f);
  sun_light._intensity = 1.f;
  sun_light._dir.Set(0.1294095, -0.9659258, 0.2241439);

  auto world = GameWorld::Instance();
  int num_camera;
  Camera* camera = world->GetCameras(&num_camera);
  if (num_camera <= 0) return;
  auto GR = GraphicsRenderer::Instance();
  u8 view;
  auto win_size = GR->GetWindowSize();
  camera->SetAspect(win_size.x / win_size.y);
  camera->SetClipPlane(1, 3000);
  auto camera_volume = camera->_frustum.ToPBVolume();

  view = GR->PushView();
  GR->SetViewFrameBuffer(view, _shadow_fb);
  GR->SetViewRect(view, 0, 0, 4096, 4096);
  GR->SetViewClear(view, C3_CLEAR_DEPTH, 0, 1.f);
  Frustum light_frustum = GetLightFrustum(&sun_light, &camera->_frustum);
  float4x4 light_view = light_frustum.ComputeViewMatrix();
  float4x4 light_proj = light_frustum.ComputeProjectionMatrix();
  GR->SetViewTransform(view, light_view.ptr(), light_proj.ptr());
  for (int i = 0; i < _num_models; ++i) {
    ModelRenderer* mr = _models + i;
    if (!mr->_asset || mr->_asset->_state != ASSET_STATE_READY) continue;
    auto transform = world->FindTransform(mr->_entity);
    if (!transform) continue;
    float4x4 m = float4x4::FromTRS(transform->_position, transform->_rotation, transform->_scale);
    SpinLockGuard lock_guard(&mr->_asset->_lock);
    if (mr->_asset->_state != ASSET_STATE_READY) continue;
    auto model = (Model*)mr->_asset->_header->GetData();
    for (auto part = model->_parts; part < model->_parts + model->_num_parts; ++part) {
      //if (camera_volume.InsideOrIntersects(part->_aabb.Transform(m).MinimalEnclosingAABB()) == TestOutside) continue;
      GR->SetTransform(&m);
      GR->SetVertexBuffer(model->_vb);
      GR->SetIndexBuffer(model->_ib, part->_start_index, part->_num_indices);
      GR->SetState(C3_STATE_DEPTH_WRITE | C3_STATE_DEPTH_TEST_LESS | C3_STATE_CULL_CW);
      float dist = light_frustum.Distance(m.TransformPos(part->_aabb.CenterPoint()));
      auto material = (Material*)model->_materials[part->_material_index]->_header->GetData();
      auto program = material->Apply("Forward", "Shadow");
      GR->Submit(view, program, depth_to_bits(dist));
    }
  }

  view = GR->PushView();
  GR->SetViewRect(view, 0, 0, (u16)win_size.x, (u16)win_size.y);
  GR->SetViewClear(view, C3_CLEAR_COLOR | C3_CLEAR_DEPTH, 0, 1.f);
  GR->SetViewTransform(view, camera->GetViewMatrix().ptr(), camera->GetProjectionMatrix().ptr());
  for (int i = 0; i < _num_models; ++i) {
    ModelRenderer* mr = _models + i;
    if (!mr->_asset || mr->_asset->_state != ASSET_STATE_READY) continue;
    auto transform = world->FindTransform(mr->_entity);
    if (!transform) continue;
    
    float4x4 m = float4x4::FromTRS(transform->_position, transform->_rotation, transform->_scale);
    SpinLockGuard lock_guard(&mr->_asset->_lock);
    if (mr->_asset->_state != ASSET_STATE_READY) continue;
    auto model = (Model*)mr->_asset->_header->GetData();
    for (auto part = model->_parts; part < model->_parts + model->_num_parts; ++part) {
      if (camera_volume.InsideOrIntersects(part->_aabb.Transform(m).MinimalEnclosingAABB()) == TestOutside) continue;
      ApplyLight(&sun_light, &light_frustum);
      GR->SetTransform(&m);
      GR->SetVertexBuffer(model->_vb);
      GR->SetIndexBuffer(model->_ib, part->_start_index, part->_num_indices);
      GR->SetTexture(15, _shadow_fb, 0, C3_TEXTURE_COMPARE_LESS);
      GR->SetState(C3_STATE_RGB_WRITE | C3_STATE_ALPHA_WRITE | C3_STATE_DEPTH_WRITE |
                    C3_STATE_CULL_CW | C3_STATE_DEPTH_TEST_LEQUAL);
      float dist = camera->_frustum.Distance(m.TransformPos(part->_aabb.CenterPoint()));
      auto material = (Material*)model->_materials[part->_material_index]->_header->GetData();
      auto program = material->Apply("Forward", "Geometry");
      GR->Submit(view, program, depth_to_bits(dist));
    }
  }
}

void RenderSystem::ApplyLight(Light* light, Frustum* light_frustum) {
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
  
  float4x4 m = float4x4::Translate(0.5f, 0.5f, 0.f) * float4x4::Scale(0.5f, -0.5f, 1.f) * light_frustum->ComputeViewProjMatrix();
  GR->SetConstant(_constant_light_transform, &m);
}

Frustum RenderSystem::GetLightFrustum(Light* light, Frustum* camera_frustum) const {
  vec points[8];
  camera_frustum->GetCornerPoints(points);
  OBB obb;
  light->_dir.PerpendicularBasis(obb.axis[0], obb.axis[1]);
  obb.axis[2] = light->_dir;

  for (int i = 0; i < _num_models; ++i) {
    auto mr = _models + i;
    if (!mr->_asset || mr->_asset->_state != ASSET_STATE_READY) continue;
    auto transform = GameWorld::Instance()->FindTransform(mr->_entity);
    if (!transform) continue;
    float4x4 m = float4x4::FromTRS(transform->_position, transform->_rotation, transform->_scale);
    auto model = (Model*)mr->_asset->_header->GetData();
    model->_aabb.GetCornerPoints(points);
    auto r0 = obb.axis[0];
    auto r1 = obb.axis[1];
    obb = OBB::FixedOrientationEnclosingOBB(points, 8, r0, r1);
  }
  Frustum light_frustum;
  light_frustum.SetKind(FrustumSpaceD3D, FrustumRightHanded);
  light_frustum.SetFrame(obb.pos - obb.r[2] * obb.axis[2], obb.axis[2], obb.axis[1]);
  light_frustum.SetOrthographic(obb.r[0] * 2.f, obb.r[1] * 2.f);
  light_frustum.SetViewPlaneDistances(0.f, obb.r[2] * 2.f);
  return light_frustum;
}
