#pragma once
#include "Data/DataType.h"
#include "Graphics/Color.h"

enum LightType {
  DIRECTIONAL_LIGHT,
  POINT_LIGHT,
  SPOT_LIGHT,
};

struct DirectionalLightData {
  float3 _dir;
};

struct PointLightData {
  float3 _pos;
  float2 _dist_falloff;
};

struct SpotLightData {
  float3 _pos;
  float3 _dir;
  float2 _dist_falloff;
  float2 _angle_falloff;
};

struct Light {
  EntityHandle _entity;
  LightType _type;
  Color _color;
  float _intensity;
  bool _cast_shadow;
  float3 _pos;
  float3 _dir;
  float2 _dist_falloff;
  float2 _angle_falloff;

  Light() { Init(); }

  void Init() {
    _type = DIRECTIONAL_LIGHT;
    _color = Color::WHITE;
    _intensity = 1.f;
    _cast_shadow = false;
    _pos = float3::zero;
    _dir = -float3::unitY;
    _dist_falloff.Set(0.f, 10.f);
    _angle_falloff.Set(0.f, DegToRad(30.f));
  }
  void GetViewProjectionMatrix(float4x4& view, float4x4& proj) const;
};
