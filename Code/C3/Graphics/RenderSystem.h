#pragma once

#include "Data/DataType.h"
#include "ECS/System.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Light/Light.h"

class RenderSystem : public ISystem {
public:
  RenderSystem();
  ~RenderSystem();

  bool OwnComponentType(ComponentType type) const override;
  void CreateComponent(EntityHandle entity, ComponentType type) override;
  void SerializeComponents(BlobWriter& writer) override;
  void DeserializeComponents(BlobReader& reader, EntityResourceDeserializeContext& ctx) override;

  ModelRenderer* CreateModelRenderer(EntityHandle e);
  void DestroyModelRenderer(EntityHandle e);
  const char* GetModelFilename(EntityHandle e) const;
  void SetModelFilename(EntityHandle e, const char* filename);
  ModelRenderer* FindModel(EntityHandle e) const;

  Light* CreateLight(EntityHandle e);
  void DestroyLight(EntityHandle e);
  void SetLightType(EntityHandle e, LightType type);
  void SetLightColor(EntityHandle e, const Color& color);
  void SetLightIntensity(EntityHandle e, float intensity);
  void SetLightCastShadow(EntityHandle e, bool shadow);
  void SetLightDir(EntityHandle e, const float3& dir);
  void SetLightPos(EntityHandle e, const float3& pos);
  void SetLightDistFalloff(EntityHandle e, const float2& fallof);
  void SetLightAngleFalloff(EntityHandle e, const float2& fallof);
  LightType GetLightType(EntityHandle e) const;
  Color GetLightColor(EntityHandle e) const;
  float GetLightIntensity(EntityHandle e) const;
  bool GetLightCastShadow(EntityHandle e) const;
  float3 GetLightDir(EntityHandle e) const;
  float3 GetLightPos(EntityHandle e, const float3& pos) const;
  float2 GetLightDistFalloff(EntityHandle e);
  float2 GetLightAngleFalloff(EntityHandle e);
  Light* FindLight(EntityHandle e) const;
  Light* GetLights(int* num_lights) const;

  void Render(float dt, bool paused) override;

private:
  void ApplyLight(Light* light, Frustum* light_frustum);
  Frustum GetLightFrustum(Light* light, Frustum* camera_frustum) const;
  void SerializeModels(BlobWriter& writer);
  void SerializeLights(BlobWriter& writer);
  void DeserializeModels(BlobReader& reader, EntityResourceDeserializeContext& ctx);
  void DeserializeLights(BlobReader& reader, EntityResourceDeserializeContext& ctx);

  ModelRenderer _models[C3_MAX_MODEL_RENDERERS];
  int _num_models;
  unordered_map<EntityHandle, int> _model_map;

  Light _lights[C3_MAX_LIGHTS];
  int _num_lights;
  unordered_map<EntityHandle, int> _light_map;

  ConstantHandle _constant_light_type;
  ConstantHandle _constant_light_color;
  ConstantHandle _constant_light_pos;
  ConstantHandle _constant_light_dir;
  ConstantHandle _constant_light_falloff;
  ConstantHandle _constant_light_transform;

  FrameBufferHandle _shadow_fb;
};
