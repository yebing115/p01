#include "C3PCH.h"
#include "GameWorld.h"
#include "System.h"

DEFINE_SINGLETON_INSTANCE(GameWorld);
IMPLEMENT_REFLECT(GameWorld);

GameWorld::GameWorld() {
  INIT_LIST_HEAD(&_entity_list);
  _num_cameras = 0;
  _num_transforms = 0;
}
GameWorld::~GameWorld() {}

EntityHandle GameWorld::CreateEntity() {
  auto h = _entity_alloc.Alloc();
  if (h) {
    _entities[h.idx].Init();
    list_add_tail(&_entities[h.idx]._sibling_link, &_entity_list);
  }
  return h;
}

EntityHandle GameWorld::CreateEntity(EntityHandle parent) {
  if (!parent) {
    c3_log("CreateEntity: Invalid parent entity, idx = %d\n", parent.idx);
    return EntityHandle();
  }
  auto& p = _entities[parent.idx];
  auto h = _entity_alloc.Alloc();
  if (h) {
    _entities[h.idx].Init();
    _entities[h.idx]._parent = &p;
    list_add_tail(&_entities[h.idx]._sibling_link, &p._child_list);
  }
  return h;
}

void GameWorld::DestroyEntity(EntityHandle e) {
  if (!e) {
    c3_log("DestroyEntity: Invalid entity, idx = %d\n", e.idx);
    return;
  }
  DestroyEntity(_entities + e.idx);
}

void GameWorld::DestroyEntity(Entity* e) {
  Entity* child, *tmp;
  list_for_each_entry_safe(child, tmp, &e->_child_list, _sibling_link) {
    DestroyEntity(child);
  }
  c3_assert(list_empty(&e->_child_list));
  e->_parent = nullptr;
  list_del(&e->_sibling_link);
}

ISystem* GameWorld::GetSystem(HandleType type) const {
  if (OwnComponentType(type)) return (ISystem*)this;
  for (auto sys : _systems) {
    if (sys->OwnComponentType(type)) return sys;
  }
  return nullptr;
}

void GameWorld::Update(float dt, bool paused) {
  for (auto& sys : _systems) sys->Update(dt, paused);
}

void GameWorld::Render(float dt, bool paused) {
  for (auto& sys : _systems) sys->Render(dt, paused);
}

bool GameWorld::OwnComponentType(HandleType type) const {
  return (type == TRANSFORM_HANDLE || type == CAMERA_HANDLE);
}

void GameWorld::CreateComponent(EntityHandle entity, HandleType type) {
  auto sys = GetSystem(type);
  if (sys) sys->CreateComponent(entity, type);
}

void GameWorld::CreateCamera(EntityHandle e) {
  c3_assert_return(_num_cameras < C3_MAX_CAMERAS);
  Camera* camera = _cameras + _num_cameras;
  camera->_entity = e;
  camera->Init();
  _camera_map.insert(EntityMap::value_type(e, _num_cameras));
  ++_num_cameras;
}

void GameWorld::DestroyCamera(EntityHandle e) {
  auto it = _camera_map.find(e);
  if (it != _camera_map.end()) {
    auto index = it->second;
    --_num_cameras;
    if (index != _num_cameras) {
      memcpy(_cameras + index, _cameras + _num_cameras, sizeof(Camera));
      auto moved_entity = _cameras[index]._entity;
      _camera_map[moved_entity] = index;
    }
    _camera_map.erase(e);
  }
}

void GameWorld::SetCameraVerticalFovAndAspectRatio(EntityHandle e, float v_fov, float aspect) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->SetVerticalFovAndAspectRatio(v_fov, aspect);
}

void GameWorld::SetCameraPos(EntityHandle e, const vec& pos) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->_frustum.SetPos(pos);
}

float3 GameWorld::GetCameraPos(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return float3::zero;
  return camera->_frustum.Pos();
}

void GameWorld::SetCameraFront(EntityHandle e, const vec& front) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->_frustum.SetFront(front);
}

float3 GameWorld::GetCameraFront(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return -float3::unitZ;
  return camera->_frustum.Front();
}

void GameWorld::SetCameraUp(EntityHandle e, const vec& up) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->_frustum.SetUp(up);
}

float3 GameWorld::GetCameraUp(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return float3::unitY;
  camera->_frustum.Up();
}

float3 GameWorld::GetCameraRight(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return float3::unitX;
  return camera->GetRight();
}

void GameWorld::SetCameraClipPlane(EntityHandle e, float near, float far) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->SetClipPlane(near, far);
}

float GameWorld::GetCameraNear(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return 0.f;
  return camera->GetNear();
}

float GameWorld::GetCameraFar(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return 0.f;
  return camera->GetFar();
}

float GameWorld::GetDistance(EntityHandle e, const vec& p) const {
  auto camera = FindCamera(e);
  if (!camera) return 0.f;
  return camera->_frustum.Distance(p);
}

void GameWorld::PanCamera(EntityHandle e, float dx, float dy) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->Pan(dx, dy);
}

void GameWorld::TransformCamera(EntityHandle e, const Quat& q) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->Transform(q);
}

void GameWorld::ZoomCamera(EntityHandle e, float zoom_factor) {
  auto camera = FindCamera(e);
  if (!camera) return;
  camera->Zoom(zoom_factor);
}

float GameWorld::GetCameraVerticalFov(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return 0.f;
  return camera->_frustum.VerticalFov();
}

float GameWorld::GetCameraAspect(EntityHandle e) const {
  auto camera = FindCamera(e);
  if (!camera) return 1.f;
  return camera->_frustum.AspectRatio();
}

Camera* GameWorld::FindCamera(EntityHandle e) const {
  auto it = _camera_map.find(e);
  return (it == _camera_map.end()) ? nullptr : (Camera*)_cameras + it->second;
}

Camera* GameWorld::GetCameras(int* num_cameras) const {
  *num_cameras = _num_cameras;
  return (Camera*)_cameras;
}

void GameWorld::CreateTransform(EntityHandle e) {
  c3_assert_return(_num_transforms < C3_MAX_TRANSFORMS);
  Transform* transform = _transforms + _num_transforms;
  transform->_entity = e;
  transform->Init();
  _transform_map.insert(EntityMap::value_type(e, _num_transforms));
  ++_num_transforms;
}

void GameWorld::DestroyTransform(EntityHandle e) {
  auto it = _transform_map.find(e);
  if (it != _transform_map.end()) {
    auto index = it->second;
    --_num_transforms;
    if (index != _num_transforms) {
      memcpy(_transforms + index, _transforms + _num_transforms, sizeof(Transform));
      auto moved_entity = _transforms[index]._entity;
      _transform_map[moved_entity] = index;
    }
    _transform_map.erase(e);
  }
}

Transform* GameWorld::FindTransform(EntityHandle e) const {
  auto it = _transform_map.find(e);
  return (it == _transform_map.end()) ? nullptr : (Transform*)_transforms + it->second;
}

Transform* GameWorld::GetTransforms(int* num_transforms) const {
  *num_transforms = _num_transforms;
  return (Transform*)_transforms;
}
