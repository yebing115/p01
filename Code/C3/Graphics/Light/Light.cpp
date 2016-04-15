#include "C3PCH.h"
#include "Light.h"

void Light::GetViewProjectionMatrix(float4x4& view, float4x4& proj) const {
  Frustum light_frustum;
  light_frustum.SetKind(FrustumSpaceD3D, FrustumRightHanded);
  light_frustum.SetPos(_pos);
  light_frustum.SetFrame(_pos, _dir, _dir.Perpendicular());
  light_frustum.SetOrthographic(2000.f, 2000.f);
  view = light_frustum.ComputeViewMatrix();
  proj = light_frustum.ComputeProjectionMatrix();
}
