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
  for (auto sys : _systems) {
    if (sys->OwnHandleType(type)) return sys;
  }
  return nullptr;
}

void GameWorld::RegisterReflectInfos() {
  //ComponentRegistry::SetName(CAMERA_COMPONENT, "Camera");
  //ComponentRegistry::Add(CAMERA_COMPONENT, );
}

CameraHandle GameWorld::CreateCamera(EntityHandle eh) {
  auto h = _camera_handles.Alloc();
  if (h) {
    _cameras[h.idx]._entity = eh;
    _cameras[h.idx].Init();
  }
  return h;
}

void GameWorld::DestroyCamera(CameraHandle handle) {
  _camera_handles.Free(handle);
}

TransformHandle GameWorld::CreateTransform(EntityHandle eh) {
  auto h = _transform_handles.Alloc();
  if (h) {
    _transforms[h.idx]._entity = eh;
    _transforms[h.idx].Init();
  }
  return h;
}

void GameWorld::DestroyTransform(TransformHandle handle) {
  _transform_handles.Free(handle);
}
