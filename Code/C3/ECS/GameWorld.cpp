#include "C3PCH.h"
#include "GameWorld.h"
#include "System.h"
#include "ECS/EntityResource.h"

DEFINE_SINGLETON_INSTANCE(GameWorld);
IMPLEMENT_REFLECT(GameWorld);

GameWorld::GameWorld() {
  _num_name_annotations = 0;
  _num_cameras = 0;
  _num_transforms = 0;
  INIT_LIST_HEAD(&_entity_list);
  _entity_dense_map_dirty = true;
}
GameWorld::~GameWorld() {}

EntityHandle GameWorld::CreateEntity() {
  auto h = _entity_alloc.Alloc();
  if (h) {
    _entities[h.idx].Init();
    list_add_tail(&_entities[h.idx]._sibling_link, &_entity_list);
    _entity_dense_map_dirty = true;
  }
  return h;
}

EntityHandle GameWorld::CreateEntity(EntityHandle parent) {
  if (!_entity_alloc.IsValid(parent)) {
    c3_log("CreateEntity: Invalid parent entity, idx = %d\n", parent.idx);
    return EntityHandle();
  }
  auto h = _entity_alloc.Alloc();
  if (h) {
    _entities[h.idx].Init();
    _entities[h.idx]._parent = parent;
    list_add_tail(&_entities[h.idx]._sibling_link, &_entities[parent.idx]._child_list);
    _entity_dense_map_dirty = true;
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
  e->_parent = EntityHandle();
  list_del(&e->_sibling_link);
  _entity_dense_map_dirty = true;
}

void GameWorld::SetEntityParent(EntityHandle e, EntityHandle parent) {
  if (!_entity_alloc.IsValid(e)) return;
  Entity* ent = _entities + e.idx;
  Entity* old_pent = nullptr;
  Entity* pent = nullptr;
  if (ent->_parent) old_pent = _entities + ent->_parent.idx;
  if (parent) pent = _entities + parent.idx;
  if (old_pent) list_del_init(&ent->_sibling_link);
  if (pent) list_add_tail(&ent->_sibling_link, &pent->_child_list);
}

void GameWorld::SerializeEntities(BlobWriter& writer) {
  u32 n = _entity_alloc.GetUsed();
  u32* parent = (u32*)C3_ALLOC(g_allocator, sizeof(u32) * n);
  auto ehs = _entity_alloc.GetPointer();
  for (u32 i = 0; i < n; ++i) {
    Entity* e = _entities + ehs[i].idx;
    parent[i] = GetEntityDenseIndex(e->_parent);
  }
  writer.Write(parent, sizeof(u32) * n);
  C3_FREE(g_allocator, parent);
}

void GameWorld::SerializeTransforms(BlobWriter& writer) {
  ComponentTypeResourceHeader header;
  header._type = TRANSFORM_COMPONENT;
  header._size = sizeof(header) + sizeof(Transform) * _num_transforms;
  header._num_entities = _num_transforms;
  header._data_offset = writer.GetPos() + sizeof(header);
  writer.Write(header);
  void* data = nullptr;
  writer.Write(_transforms, sizeof(Transform) * _num_transforms, &data);
  for (u32 i = 0; i < _num_transforms; ++i) {
    auto t = (Transform*)data + i;
    t->_entity.idx = GetEntityDenseIndex(t->_entity);
  }
}

void GameWorld::SerializeCameras(BlobWriter& writer) {
  ComponentTypeResourceHeader header;
  header._type = CAMERA_COMPONENT;
  header._size = sizeof(header) + sizeof(Camera) * _num_cameras;
  header._num_entities = _num_cameras;
  header._data_offset = writer.GetPos() + sizeof(header);
  writer.Write(header);
  void* data = nullptr;
  writer.Write(_cameras, sizeof(Camera) * _num_cameras, &data);
  for (u32 i = 0; i < _num_transforms; ++i) {
    auto c = (Camera*)data + i;
    c->_entity.idx = GetEntityDenseIndex(c->_entity);
  }
}

void GameWorld::SerializeNameAnnotations(BlobWriter& writer) {
  ComponentTypeResourceHeader header;
  header._type = NAME_ANNOTATION_COMPONENT;
  header._size = sizeof(header) + sizeof(NameAnnotation) * _num_name_annotations;
  header._num_entities = _num_name_annotations;
  header._data_offset = writer.GetPos() + sizeof(header);
  writer.Write(header);
  void* data = nullptr;
  writer.Write(_transforms, sizeof(NameAnnotation) * _num_name_annotations, &data);
  for (u32 i = 0; i < _num_name_annotations; ++i) {
    auto name_anno = (NameAnnotation*)data + i;
    name_anno->_entity.idx = GetEntityDenseIndex(name_anno->_entity);
  }
}

void GameWorld::DeserializeTransforms(BlobReader& reader, EntityResourceDeserializeContext& ctx) {
  ComponentTypeResourceHeader header;
  reader.Read(header);
  reader.Seek(header._data_offset);
  u32 n = header._num_entities;
  for (u32 i = 0; i < n; ++i) {
    u32 idx;
    reader.Peek(idx);
    Transform* t = CreateTransform(ctx._entities[idx]);
    reader.Read(t, sizeof(Transform));
    t->_entity = ctx._entities[idx];
  }
}

void GameWorld::DeserializeCameras(BlobReader& reader, EntityResourceDeserializeContext& ctx) {
  ComponentTypeResourceHeader header;
  reader.Read(header);
  reader.Seek(header._data_offset);
  u32 n = header._num_entities;
  for (u32 i = 0; i < n; ++i) {
    u32 idx;
    reader.Peek(idx);
    Camera* c = CreateCamera(ctx._entities[idx]);
    reader.Read(c, sizeof(Camera));
    c->_entity = ctx._entities[idx];
  }
}

void GameWorld::DeserializeNameAnnotations(BlobReader& reader, EntityResourceDeserializeContext& ctx) {
  ComponentTypeResourceHeader header;
  reader.Read(header);
  reader.Seek(header._data_offset);
  u32 n = header._num_entities;
  for (u32 i = 0; i < n; ++i) {
    NameAnnotation name_anno;
    reader.Peek(name_anno);
    SetEntityName(ctx._entities[name_anno._entity.ToRaw()], name_anno._name);
  }
}

int GameWorld::GetEntityDenseIndex(EntityHandle e) const {
  if (_entity_dense_map_dirty) {
    _entity_dense_map.clear();
    u32 n = _entity_alloc.GetUsed();
    auto entities = _entity_alloc.GetPointer();
    _entity_dense_map.reserve(n);
    for (u32 i = 0; i < n; ++i) {
      _entity_dense_map[entities[i]] = i;
    }
    _entity_dense_map_dirty = false;
  }
  auto it = _entity_dense_map.find(e);
  return (it == _entity_dense_map.end()) ? -1 : it->second;
}

ISystem* GameWorld::GetSystem(ComponentType type) const {
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

void GameWorld::SerializeWorld(BlobWriter& writer) {
  u32 num_comp_types = 0;
  for (u32 i = 0; i < NUM_COMPONENT_TYPES; ++i) {
    if (GetSystem((ComponentType)i)) ++num_comp_types;
  }
  EntityResourceHeader header;
  header._magic = C3_CHUNK_MAGIC_ENT;
  header._num_asset_refs = AssetManager::Instance()->GetUsed();
  header._num_entites = _entity_alloc.GetUsed();
  header._num_component_types = num_comp_types;
  header.InitDataOffsets();
  writer.Write(header);
  
  writer.Seek(header._asset_refs_data_offset);
  AssetManager::Instance()->Serialize(writer);
  
  writer.Seek(header._entity_parents_data_offset);
  SerializeEntities(writer);
  
  writer.Seek(header._component_types_data_offset);
  SerializeComponents(writer);
  for (auto sys : _systems) sys->SerializeComponents(writer);
}

void GameWorld::DeserializeWorld(BlobReader& reader) {
  EntityResourceHeader header;
  reader.Read(header);
  c3_assert_return(header._magic == C3_CHUNK_MAGIC_ENT);
  
  EntityResourceDeserializeContext ctx;
  ctx._assets = (Asset**)C3_ALLOC(g_allocator, sizeof(Asset*) * header._num_asset_refs);
  auto AM = AssetManager::Instance();
  reader.Seek(header._asset_refs_data_offset);
  AssetDesc desc;
  for (u32 i = 0; i < header._num_asset_refs; ++i) {
    reader.Read(desc);
    ctx._assets[i] = AM->Load((AssetType)desc._type, desc._filename);
  }
  ctx._entities = (EntityHandle*)C3_ALLOC(g_allocator, sizeof(Asset*) * header._num_entites);
  for (u32 i = 0; i < header._num_entites; ++i) {
    ctx._entities[i] = CreateEntity();
  }
  reader.Seek(header._entity_parents_data_offset);
  for (u32 i = 0; i < header._num_entites; ++i) {
    u32 parent;
    reader.Read(parent);
    if (parent != UINT32_MAX) SetEntityParent(ctx._entities[i], ctx._entities[parent]);
  }
  DeserializeComponents(reader, ctx);
  for (auto sys : _systems) sys->DeserializeComponents(reader, ctx);
}

bool GameWorld::OwnComponentType(ComponentType type) const {
  return (type == TRANSFORM_COMPONENT || type == CAMERA_COMPONENT);
}

void GameWorld::CreateComponent(EntityHandle entity, ComponentType type) {
  auto sys = GetSystem(type);
  if (sys) sys->CreateComponent(entity, type);
}

void GameWorld::SerializeComponents(BlobWriter& writer) {
  SerializeTransforms(writer);
  SerializeCameras(writer);
  SerializeNameAnnotations(writer);
}

void GameWorld::DeserializeComponents(BlobReader& reader, EntityResourceDeserializeContext& ctx) {
  reader.Seek(ctx._header._component_types_data_offset);
  for (u32 i = 0; i < ctx._header._num_component_types; ++i) {
    ComponentTypeResourceHeader comp_header;
    reader.Peek(comp_header);
    if (comp_header._type == TRANSFORM_COMPONENT) DeserializeTransforms(reader, ctx);
    else if (comp_header._type == CAMERA_COMPONENT) DeserializeCameras(reader, ctx);
    else if (comp_header._type == NAME_ANNOTATION_COMPONENT) DeserializeNameAnnotations(reader, ctx);
    else reader.Skip(comp_header._size);
  }
}

void GameWorld::SetEntityName(EntityHandle e, const char* name) {
  if (!_entity_alloc.IsValid(e)) return;
  auto name_anno = FindNameAnnotation(e);
  if (!name_anno) {
    name_anno = _name_annotations + _num_name_annotations;
    _name_annotation_map[e] = _num_name_annotations;
    _num_name_annotations++;
    name_anno->_entity = e;
  }
  strncpy(name_anno->_name, name, MAX_NAME_ANNOTATION);
  name_anno->_id = String::GetID(name);
}

const char* GameWorld::GetEntityName(EntityHandle e) const {
  auto name_anno = FindNameAnnotation(e);
  return name_anno ? name_anno->_name : "";
}

EntityHandle GameWorld::FindEntityByName(const char* name) const {
  auto id = String::GetID(name);
  for (u32 i = 0; i < _num_name_annotations; ++i) {
    const NameAnnotation* name_anno = _name_annotations + i;
    if (name_anno->_id == id && strcmp(name_anno->_name, name) == 0) return name_anno->_entity;
  }
  return EntityHandle();
}

NameAnnotation* GameWorld::FindNameAnnotation(EntityHandle e) const {
  auto it = _name_annotation_map.find(e);
  return (it == _name_annotation_map.end()) ? nullptr : (NameAnnotation*)_name_annotations + it->second;
}

Camera* GameWorld::CreateCamera(EntityHandle e) {
  c3_assert_return_x(_num_cameras < C3_MAX_CAMERAS, nullptr);
  Camera* camera = _cameras + _num_cameras;
  camera->_entity = e;
  camera->Init();
  _camera_map.insert(EntityMap::value_type(e, _num_cameras));
  ++_num_cameras;
  return camera;
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

Transform* GameWorld::CreateTransform(EntityHandle e) {
  c3_assert_return_x(_num_transforms < C3_MAX_TRANSFORMS, nullptr);
  Transform* transform = _transforms + _num_transforms;
  transform->_entity = e;
  transform->Init();
  _transform_map.insert(EntityMap::value_type(e, _num_transforms));
  ++_num_transforms;
  return transform;
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
