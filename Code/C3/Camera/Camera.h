#pragma once

#include "Data/C3Data.h"

struct Camera {
  EntityHandle _entity;
  Frustum _frustum;
  float _focal_distance;

  void Init();
  void SetFrame(const vec& pos, const vec& front, const vec& up);
  void SetClipPlane(float n, float f);
  void SetOrthographic(float w, float h);
  void SetPerspective(float fov_x, float fov_y);
  void SetVerticalFovAndAspectRatio(float fov_y, float aspect);
  void SetVerticalFov(float fov_y);
  void SetAspect(float aspect);
  void SetPos(const vec& eye) { _frustum.SetPos(eye); }
  void SetFront(const vec& front) { _frustum.SetFront(front); }
  void SetUp(const vec& up) { _frustum.SetUp(up); }
  void SetFocalDistance(float f, bool update_eye = true);
  float GetFocalDistance() const { return _focal_distance; }
  vec GetFocalPoint() const;
  Plane GetFocalPlane(float2* plane_size = nullptr) const;
  float2 GetFocalPlaneSize() const;
  const vec& GetPos() const { return _frustum.Pos(); }
  const vec& GetFront() const { return _frustum.Front(); }
  const vec& GetUp() const { return _frustum.Up(); }
  vec GetRight() const { return _frustum.WorldRight(); }
  float GetNear() const { return _frustum.NearPlaneDistance(); }
  float GetFar() const { return _frustum.FarPlaneDistance(); }
  void Pan(const float2& d) { Pan(d.x, d.y); }
  void Pan(float dx, float dy);
  void Zoom(float ratio);
  void Translate(const vec& d) { _frustum.Translate(d); }
  void Translate(float dx, float dy, float dz) { Translate(vec(dx, dy, dz)); }
  void Transform(const Quat& q) { _frustum.Transform(q); }
  float4x4 GetViewMatrix() const { return _frustum.ComputeViewMatrix(); }
  float4x4 GetProjectionMatrix() const { return _frustum.ComputeProjectionMatrix(); }
  float4x4 GetWorldMatrix() const { return _frustum.ComputeWorldMatrix(); }
  float4x4 GetViewProjectionMatrix() const { return _frustum.ComputeViewProjMatrix(); }
};
