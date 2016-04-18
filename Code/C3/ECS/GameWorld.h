#pragma once

#include "Data/DataType.h"
#include "Data/Blob.h"
#include "Reflection/Object.h"
#include "ComponentTypes.h"
#include "Pattern/Singleton.h"
#include "Camera/Camera.h"
#include "Entity.h"

#define MAX_NAME_ANNOTATION 56
struct NameAnnotation {
  EntityHandle _entity;
  stringid _id;
  char _name[MAX_NAME_ANNOTATION];
};
static_assert(sizeof(NameAnnotation) == 64, "Invalid sizeof(NameAnnotation), expect 64.");

struct Transform {
  EntityHandle _entity;

  vec _position;
  Quat _rotation;
  vec _scale;

  void Init() {
    _position = vec::zero;
    _rotation = Quat::identity;
    _scale = vec::one;
  }
};

class ISystem;
class GameWorld : public ISystem {
public:
  GameWorld();
  ~GameWorld();

  // ISystem interfaces
  bool OwnComponentType(ComponentType type) const override;
  void CreateComponent(EntityHandle entity, ComponentType type) override;
  void SerializeComponents(BlobWriter& writer) override;

  // Entity
  EntityHandle CreateEntity();
  EntityHandle CreateEntity(EntityHandle parent);
  void DestroyEntity(EntityHandle e);
  int GetEntityDenseIndex(EntityHandle e) const;

  // Name
  void SetEntityName(EntityHandle e, const char* name);
  const char* GetEntityName(EntityHandle e) const;
  EntityHandle FindEntityByName(const char* name) const;
  NameAnnotation* FindNameAnnotation(EntityHandle e) const;

  // Transform
  void CreateTransform(EntityHandle e);
  void DestroyTransform(EntityHandle e);
  Transform* FindTransform(EntityHandle e) const;
  Transform* GetTransforms(int* num_transforms) const;
  
  // Camera control
  void CreateCamera(EntityHandle e);
  void DestroyCamera(EntityHandle e);
  void SetCameraVerticalFovAndAspectRatio(EntityHandle e, float v_fov, float aspect);
  void SetCameraPos(EntityHandle e, const vec& pos);
  float3 GetCameraPos(EntityHandle e) const;
  void SetCameraFront(EntityHandle e, const vec& front);
  float3 GetCameraFront(EntityHandle e) const;
  void SetCameraUp(EntityHandle e, const vec& up);
  float3 GetCameraUp(EntityHandle e) const;
  float3 GetCameraRight(EntityHandle e) const;
  void SetCameraClipPlane(EntityHandle e, float near, float far);
  float GetCameraNear(EntityHandle e) const;
  float GetCameraFar(EntityHandle e) const;
  float GetDistance(EntityHandle e, const vec& p) const;
  void PanCamera(EntityHandle e, const float2& p) { PanCamera(e, p.x, p.y); }
  void PanCamera(EntityHandle e, float dx, float dy);
  void TransformCamera(EntityHandle e, const Quat& q);
  void ZoomCamera(EntityHandle e, float zoom_factor);
  float GetCameraVerticalFov(EntityHandle e) const;
  float GetCameraAspect(EntityHandle e) const;
  Camera* FindCamera(EntityHandle e) const;
  Camera* GetCameras(int* num_cameras) const;

  void AddSystem(ISystem* system) { _systems.push_back(system); }
  ISystem* GetSystem(ComponentType type) const;

  void Update(float dt, bool paused);
  void Render(float dt, bool paused);

  void SerializeWorld(BlobWriter& writer);
  void DeserializeWorld(BlobReader& reader);

private:
  void DestroyEntity(Entity* e);
  void SerializeEntities(BlobWriter& writer);
  void SerializeTransforms(BlobWriter& writer);
  void SerializeCameras(BlobWriter& writer);
  void SerializeNameAnnotations(BlobWriter& writer);

  Entity _entities[C3_MAX_ENTITIES];
  list_head _entity_list;
  HandleAlloc<ENTITY_HANDLE, C3_MAX_ENTITIES> _entity_alloc;
  mutable unordered_map<EntityHandle, int> _entity_dense_map;
  mutable bool _entity_dense_map_dirty;
  
  NameAnnotation _name_annotations[C3_MAX_ENTITIES];
  u32 _num_name_annotations;
  EntityMap _name_annotation_map;

  Transform _transforms[C3_MAX_TRANSFORMS];
  u32 _num_transforms;
  EntityMap _transform_map;

  Camera _cameras[C3_MAX_CAMERAS];
  u32 _num_cameras;
  EntityMap _camera_map;
  
  vector<ISystem*> _systems;

  friend class RenderSystem;

  SUPPORT_REFLECT(GameWorld)
  SUPPORT_SINGLETON(GameWorld)
};
