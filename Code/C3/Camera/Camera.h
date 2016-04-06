#pragma once

#include "Data/C3Data.h"

struct Camera {
  EntityHandle _entity;
  Frustum _frustum;

  void Init();
  void SetFrame(const vec& pos, const vec& front, const vec& up);
  void SetOrthographic(float w, float h);
  void SetPerspective(float fov_x, float fov_y);
  void SetVerticalFovAndAspectRatio(float fov_y, float aspect);
  float3x4 GetViewMatirx() const { return _frustum.ViewMatrix(); }
  float4x4 GetProjectionMatrix() const { return _frustum.ProjectionMatrix(); }
  float3x4 GetWorldMatirx() const { return _frustum.WorldMatrix(); }
  float4x4 GetViewProjectionMatrix() const { return _frustum.ViewProjMatrix(); }
};
