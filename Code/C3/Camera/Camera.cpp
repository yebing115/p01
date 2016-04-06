#include "C3PCH.h"
#include "Camera.h"

void Camera::Init() {
  _frustum.SetKind(FrustumSpaceGL, FrustumRightHanded);
}

void Camera::SetFrame(const vec& pos, const vec& front, const vec& up) {
  _frustum.SetFrame(pos, front, up);
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
