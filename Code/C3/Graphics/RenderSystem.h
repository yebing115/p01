#pragma once

#include "Data/DataType.h"
#include "ECS/System.h"
#include "Graphics/Model/ModelRenderer.h"
#include "Graphics/Light/Light.h"

class RenderSystem : public ISystem {
public:
  RenderSystem();
  ~RenderSystem();

  bool OwnComponentType(HandleType type) const override;
  GenericHandle CreateComponent(EntityHandle entity, HandleType type) override;

  ModelRendererHandle CreateModelRenderer(EntityHandle entity);
  void DestroyModelRenderer(ModelRendererHandle handle);
  const char* GetModelFilename(GenericHandle gh) const;
  void SetModelFilename(GenericHandle gh, const char* filename);

  LightHandle CreateLight(EntityHandle e);
  void DestroyLight(LightHandle h);
  void SetLightType(GenericHandle gh, LightType type);
  void SetLightColor(GenericHandle gh, const Color& color);
  void SetLightIntensity(GenericHandle gh, float intensity);
  void SetLightCastShadow(GenericHandle gh, bool shadow);
  void SetLightDir(GenericHandle gh, const float3& dir);
  void SetLightPos(GenericHandle gh, const float3& pos);
  void SetLightDistFalloff(GenericHandle gh, const float2& fallof);
  void SetLightAngleFalloff(GenericHandle gh, const float2& fallof);
  LightType GetLightType(GenericHandle gh) const;
  Color GetLightColor(GenericHandle gh) const;
  float GetLightIntensity(GenericHandle gh) const;
  bool GetLightCastShadow(GenericHandle gh) const;
  float3 GetLightDir(GenericHandle gh) const;
  float3 GetLightPos(GenericHandle gh, const float3& pos) const;
  float2 GetLightDistFalloff(GenericHandle gh);
  float2 GetLightAngleFalloff(GenericHandle gh);
  void GetLights(const LightHandle*& out_handles, u32& out_num_handles, Light*& out_lights);

  void Render(float dt, bool paused) override;

private:
  void ApplyLight(Light* light);

  ModelRenderer _model_renderer[C3_MAX_MODEL_RENDERERS];
  HandleAlloc<MODEL_RENDERER_HANDLE, C3_MAX_MODEL_RENDERERS> _model_renderer_handles;
  unordered_map<EntityHandle, ModelRendererHandle> _model_renderer_map;

  Light _lights[C3_MAX_LIGHTS];
  HandleAlloc<LIGHT_HANDLE, C3_MAX_LIGHTS> _light_handles;
  unordered_map<EntityHandle, LightHandle> _light_entity_map;

  ConstantHandle _constant_light_type;
  ConstantHandle _constant_light_color;
  ConstantHandle _constant_light_pos;
  ConstantHandle _constant_light_dir;
  ConstantHandle _constant_light_falloff;
  ConstantHandle _constant_light_transform;
};
