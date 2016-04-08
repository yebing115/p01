#pragma once

#include "Data/DataType.h"
#include "Reflection/Object.h"
#include "ComponentTypes.h"
#include "Pattern/Singleton.h"
#include "Camera/Camera.h"
#include "Entity.h"

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

  EntityHandle CreateEntity();
  EntityHandle CreateEntity(EntityHandle parent);
  void DestroyEntity(EntityHandle handle);

  bool OwnComponentType(HandleType type) const override;
  GenericHandle CreateComponent(EntityHandle entity, HandleType type) override;

  TransformHandle CreateTransform(EntityHandle handle);
  void DestroyTransform(GenericHandle handle);
  
  // Camera control
  CameraHandle CreateCamera(EntityHandle handle);
  void DestroyCamera(GenericHandle handle);
  void SetCameraPos(GenericHandle handle, const vec& pos);
  const vec& GetCameraPos(GenericHandle handle) const;
  void SetCameraFront(GenericHandle handle, const vec& front);
  const vec& GetCameraFront(GenericHandle handle) const;
  void SetCameraUp(GenericHandle handle, const vec& up);
  const vec& GetCameraUp(GenericHandle handle) const;
  void SetCameraClipPlane(GenericHandle handle, float near, float far);
  float GetCameraNear(GenericHandle handle) const;
  float GetCameraFar(GenericHandle handle) const;
  float GetDistance(GenericHandle handle, const vec& p) const;

  void AddSystem(ISystem* system) { _systems.push_back(system); }
  ISystem* GetSystem(HandleType type) const;

  void Update(float dt, bool paused);
  void Render(float dt, bool paused);

private:
  void DestroyEntity(Entity* e);

  Entity _entities[C3_MAX_ENTITIES];
  HandleAlloc<ENTITY_HANDLE, C3_MAX_ENTITIES> _entity_handles;

  Transform _transforms[C3_MAX_CAMERAS];
  HandleAlloc<TRANSFORM_HANDLE, C3_MAX_TRANSFORMS> _transform_handles;
  unordered_map<EntityHandle, TransformHandle> _transform_map;

  Camera _cameras[C3_MAX_CAMERAS];
  HandleAlloc<CAMERA_HANDLE, C3_MAX_CAMERAS> _camera_handles;
  unordered_map<EntityHandle, CameraHandle> _camera_map;
  
  vector<ISystem*> _systems;
  list_head _entity_list;

  friend class RenderSystem;

  SUPPORT_REFLECT(GameWorld)
  SUPPORT_SINGLETON(GameWorld)
};
