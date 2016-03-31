#include "C3PCH.h"
#include "ViewState.h"
#include "Game/GameApplication.h"

void ViewState::Reset(RenderFrame* render, bool hmd_enabled) {
  _alpha_ref = 0.0f;
  _inv_view_cached = UINT16_MAX;
  _inv_proj_cached = UINT16_MAX;
  _inv_view_proj_cached = UINT16_MAX;

  _view[0] = render->_view;
  _view[1] = _view_tmp[1];

  //auto APP = GameApplication::Instance();
  //_time = APP ? float(APP->GetWorldTime()) : 0;
  _time = 0.f;

#if 0
  if (hmd_enabled) {
    HMD& hmd = render->m_hmd;

    _view[0] = _view_tmp[0];
    float4x4 viewAdjust;
    bx::mtxIdentity(viewAdjust.un.val);

    for (u32 eye = 0; eye < 2; ++eye) {
      const HMD::Eye& hmdEye = hmd.eye[eye];
      viewAdjust.un.val[12] = hmdEye.viewOffset[0];
      viewAdjust.un.val[13] = hmdEye.viewOffset[1];
      viewAdjust.un.val[14] = hmdEye.viewOffset[2];

      for (u32 ii = 0; ii < C3_MAX_VIEWS; ++ii) {
        if ((render->view_flags[ii] & C3_VIEW_STEREO) == C3_VIEW_STEREO) {
          bx::float4x4_mul(&_view[eye][ii].un.f4x4
                           , &render->view[ii].un.f4x4
                           , &viewAdjust.un.f4x4
                           );
        } else {
          memcpy(&_view[0][ii].un.f4x4, &render->view[ii].un.f4x4, sizeof(float4x4));
        }
      }
    }
  }
#endif
  for (u32 i = 0; i < C3_MAX_VIEWS; ++i) {
    for (u32 eye = 0; eye < u32(hmd_enabled) + 1; ++eye) {
      _view_proj[eye][i] = _view[eye][i] * render->proj[eye][i];
    }
  }
}
