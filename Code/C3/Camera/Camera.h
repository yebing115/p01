#pragma once

#include "Data/C3Data.h"

struct Camera {
  EntityHandle _entity;
  Frustum _frustum;

  void Init();
  void SetFrame(const vec& pos, const vec& front, const vec& up);
  void SetClipPlane(float n, float f);
  void SetOrthographic(float w, float h);
  void SetPerspective(float fov_x, float fov_y);
  void SetVerticalFovAndAspectRatio(float fov_y, float aspect);
  float4x4 GetViewMatrix() const { return _frustum.ViewMatrix(); }
  float4x4 GetProjectionMatrix() const { return _frustum.ProjectionMatrix(); }
  float4x4 GetWorldMatrix() const { return _frustum.WorldMatrix(); }
  float4x4 GetViewProjectionMatrix() const { return _frustum.ViewProjMatrix(); }
};
