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
  void DestroyEntity(EntityHandle e);

  bool OwnComponentType(HandleType type) const override;
  void CreateComponent(EntityHandle entity, HandleType type) override;

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
  ISystem* GetSystem(HandleType type) const;

  void Update(float dt, bool paused);
  void Render(float dt, bool paused);

private:
  void DestroyEntity(Entity* e);

  Entity _entities[C3_MAX_ENTITIES];
  list_head _entity_list;
  HandleAlloc<ENTITY_HANDLE, C3_MAX_ENTITIES> _entity_alloc;

  Transform _transforms[C3_MAX_TRANSFORMS];
  int _num_transforms;
  EntityMap _transform_map;

  Camera _cameras[C3_MAX_CAMERAS];
  int _num_cameras;
  EntityMap _camera_map;
  
  vector<ISystem*> _systems;

  friend class RenderSystem;

  SUPPORT_REFLECT(GameWorld)
  SUPPORT_SINGLETON(GameWorld)
};
