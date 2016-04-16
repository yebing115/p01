#include "C3PCH.h"
#include "Camera.h"

void Camera::Init() {
  _frustum.SetKind(FrustumSpaceD3D, FrustumRightHanded);
  _focal_distance = 100.f;
  SetFrame(float3::zero, -float3::unitZ, float3::unitY);
  SetPerspective(10.f, 10.f);
  SetClipPlane(1.f, 100.f);
}

void Camera::SetFrame(const vec& pos, const vec& front, const vec& up) {
  _frustum.SetFrame(pos, front, up);
}

void Camera::SetClipPlane(float n, float f) {
  _frustum.SetViewPlaneDistances(n, f);
}

void Camera::SetOrthographic(float w, float h) {
  _frustum.SetOrthographic(w, h);
}

void Camera::SetPerspective(float fov_x, float fov_y) {
  _frustum.SetPerspective(fov_x, fov_y);
}

void Camera::SetVerticalFovAndAspectRatio(float fov_y, float aspect) {
  _frustum.SetVerticalFovAndAspectRatio(fov_y, aspect);

}

void Camera::SetVerticalFov(float fov_y) {
  auto aspect = _frustum.AspectRatio();
  _frustum.SetVerticalFovAndAspectRatio(fov_y, aspect);
}

void Camera::SetAspect(float aspect) {
  float v_fov = _frustum.VerticalFov();
  _frustum.SetVerticalFovAndAspectRatio(v_fov, aspect);
}

void Camera::SetFocalDistance(float f, bool update_pos) {
  f = Clamp(f, _frustum.NearPlaneDistance(), _frustum.FarPlaneDistance());
  if (!Equal(f, _focal_distance) && update_pos) {
    auto d = f - _focal_distance;
    _focal_distance = f;
    _frustum.SetPos(_frustum.Pos() - d * _frustum.Front());
  }
}

vec Camera::GetFocalPoint() const {
  return _frustum.Pos() + _frustum.Front() * _focal_distance;
}

Plane Camera::GetFocalPlane(float2* plane_size) const {
  if (plane_size) *plane_size = GetFocalPlaneSize();
  return Plane(GetFocalPoint(), -_frustum.Front());
}

float2 Camera::GetFocalPlaneSize() const {
  float2 size(_frustum.NearPlaneWidth(), _frustum.NearPlaneHeight());
  size *= _focal_distance / _frustum.NearPlaneDistance();
  return size;
}

void Camera::Pan(float dx, float dy) {
  auto p = _frustum.Pos() + dx * _frustum.WorldRight() + dy * _frustum.Up();
  _frustum.SetPos(p);
}

void Camera::Zoom(float ratio) {
  _frustum.SetPos(_frustum.Pos() + _frustum.Front() * (1.f - 1.f / ratio) * _focal_distance);
}
