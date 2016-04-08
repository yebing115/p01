#include "C3PCH.h"
#include "GameWorld.h"
#include "System.h"

DEFINE_SINGLETON_INSTANCE(GameWorld);
IMPLEMENT_REFLECT(GameWorld);

GameWorld::GameWorld() {
  INIT_LIST_HEAD(&_entity_list);
}
GameWorld::~GameWorld() {}

EntityHandle GameWorld::CreateEntity() {
  auto h = _entity_handles.Alloc();
  if (h) {
    _entities[h.idx].Init();
    list_add_tail(&_entities[h.idx]._sibling_link, &_entity_list);
  }
  return h;
}

EntityHandle GameWorld::CreateEntity(EntityHandle parent) {
  if (!parent) {
    c3_log("CreateEntity: Invalid parent entity, idx = %d\n", parent.IsValid());
    return EntityHandle();
  }
  auto& p = _entities[parent.idx];
  auto h = _entity_handles.Alloc();
  if (h) {
    _entities[h.idx].Init();
    _entities[h.idx]._parent = &p;
    list_add_tail(&_entities[h.idx]._sibling_link, &p._child_list);
  }
  return h;
}

void GameWorld::DestroyEntity(EntityHandle handle) {
  if (!handle) {
    c3_log("DestroyEntity: Invalid entity, idx = %d\n", handle.IsValid());
    return;
  }
  DestroyEntity(_entities + handle.idx);
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
  int n = _camera_handles.GetUsed();
  for (int i = 0; i < n; ++i) {
    auto& camera = _cameras[_camera_handles.GetHandleAt(i).idx];
    auto c = _camera_handles.GetHandleAt(i);
    if (!ImGui::GetIO().WantCaptureKeyboard) {
      if (InputManager::Instance()->IsKeyPressed(VK_W)) {
        SetCameraPos(c, GetCameraPos(c) + vec::unitY * dt * 10.f);
      }
    }
    if (!ImGui::GetIO().WantCaptureMouse) {
    }
  }
  for (auto& sys : _systems) sys->Update(dt, paused);
}

void GameWorld::Render(float dt, bool paused) {
  for (auto& sys : _systems) sys->Render(dt, paused);
}

bool GameWorld::OwnComponentType(HandleType type) const {
  return (type == TRANSFORM_HANDLE || type == CAMERA_HANDLE);
}

GenericHandle GameWorld::CreateComponent(EntityHandle entity, HandleType type) {
  auto sys = GetSystem(type);
  if (sys) return sys->CreateComponent(entity, type);
  return GenericHandle();
}

CameraHandle GameWorld::CreateCamera(EntityHandle eh) {
  auto h = _camera_handles.Alloc();
  if (h) {
    _camera_map.insert(make_pair(eh, h));
    _cameras[h.idx]._entity = eh;
    _cameras[h.idx].Init();
    _cameras[h.idx].SetFrame(vec(0, 2, 50), vec(0, 0, -1), vec(0, 1, 0));
    _cameras[h.idx].SetPerspective(DegToRad(25.f), DegToRad(25.f));
    _cameras[h.idx].SetClipPlane(1.f, 1000.f);
  }
  return h;
}

void GameWorld::DestroyCamera(GenericHandle gh) {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  _camera_map.erase(_cameras[handle.idx]._entity);
  _camera_handles.Free(handle);
}

void GameWorld::SetCameraPos(GenericHandle gh, const vec& pos) {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  _cameras[handle.idx]._frustum.SetPos(pos);
}

const MATH_NAMESPACE_NAME::vec& GameWorld::GetCameraPos(GenericHandle gh) const {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  return _cameras[handle.idx]._frustum.Pos();
}

void GameWorld::SetCameraFront(GenericHandle gh, const vec& front) {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  _cameras[handle.idx]._frustum.SetFront(front);
}

const MATH_NAMESPACE_NAME::vec& GameWorld::GetCameraFront(GenericHandle gh) const {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  return _cameras[handle.idx]._frustum.Front();
}

void GameWorld::SetCameraUp(GenericHandle gh, const vec& up) {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  _cameras[handle.idx]._frustum.SetUp(up);
}

const MATH_NAMESPACE_NAME::vec& GameWorld::GetCameraUp(GenericHandle gh) const {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  return _cameras[handle.idx]._frustum.Up();
}

void GameWorld::SetCameraClipPlane(GenericHandle gh, float near, float far) {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  _cameras[handle.idx]._frustum.SetViewPlaneDistances(near, far);
}

float GameWorld::GetCameraNear(GenericHandle gh) const {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  return _cameras[handle.idx]._frustum.NearPlaneDistance();
}

float GameWorld::GetCameraFar(GenericHandle gh) const {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  return _cameras[handle.idx]._frustum.FarPlaneDistance();
}

float GameWorld::GetDistance(GenericHandle gh, const vec& p) const {
  auto handle = cast_to<CAMERA_HANDLE>(gh, _camera_handles, _camera_map);
  return _cameras[handle.idx]._frustum.Distance(p);
}

TransformHandle GameWorld::CreateTransform(EntityHandle eh) {
  auto h = _transform_handles.Alloc();
  if (h) {
    _transform_map.insert(make_pair(eh, h));
    _transforms[h.idx]._entity = eh;
    _transforms[h.idx].Init();
  }
  return h;
}

void GameWorld::DestroyTransform(GenericHandle gh) {
  auto handle = cast_to<TRANSFORM_HANDLE>(gh, _transform_handles, _transform_map);
  _transform_map.erase(_transforms[handle.idx]._entity);
  _transform_handles.Free(handle);
}
