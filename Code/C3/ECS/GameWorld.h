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

  bool OwnComponentType(HandleType type) override;
  GenericHandle CreateComponent(EntityHandle entity, HandleType type) override;

  TransformHandle CreateTransform(EntityHandle handle);
  void DestroyTransform(TransformHandle handle);
  CameraHandle CreateCamera(EntityHandle handle);
  void DestroyCamera(CameraHandle handle);

  void AddSystem(ISystem* system) { _systems.push_back(system); }
  ISystem* GetSystem(HandleType type) const;

private:
  void DestroyEntity(Entity* e);

  Entity _entities[C3_MAX_ENTITIES];
  HandleAlloc<ENTITY_HANDLE, C3_MAX_ENTITIES> _entity_handles;

  Transform _transforms[C3_MAX_CAMERAS];
  HandleAlloc<TRANSFORM_HANDLE, C3_MAX_TRANSFORMS> _transform_handles;

  Camera _cameras[C3_MAX_CAMERAS];
  HandleAlloc<CAMERA_HANDLE, C3_MAX_CAMERAS> _camera_handles;

  vector<ISystem*> _systems;
  list_head _entity_list;

  SUPPORT_REFLECT(GameWorld)
  SUPPORT_SINGLETON(GameWorld)
};
