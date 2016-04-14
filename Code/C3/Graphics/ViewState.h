#pragma once
#include "Data/DataType.h"
#include "Graphics/RenderFrame.h"

struct RenderFrame;
struct ViewState {
  ViewState() {}
  ViewState(RenderFrame* render, bool hmd_enabled) { Reset(render, hmd_enabled); }
  void Reset(RenderFrame* render, bool hmd_enabled);
  template<u16 MaxRegCount, typename RendererContext, typename Program, typename Draw>
  void SetPredefined(RendererContext* renderer, u16 view, u8 eye, Program& program, RenderFrame* render, const Draw& draw) {
    for (u32 ii = 0, num = program._num_predefined; ii < num; ++ii) {
      PredefinedConstant& predefined = program._predefined[ii];
      u8 flags = predefined.type & CONSTANT_FRAGMENTBIT;
      switch (predefined.type & ~CONSTANT_FRAGMENTBIT) {
      case PREDEFINED_CONSTANT_VIEW_RECT: {
        float frect[4];
        frect[0] = (float)_rect.left;
        frect[1] = (float)_rect.top;
        frect[2] = (float)_rect.Width();
        frect[3] = (float)_rect.Height();

        renderer->_SetConstantVector4(flags, predefined.loc, &frect[0], 1);
      } break;
      case PREDEFINED_CONSTANT_VIEW_TEXEL: {
        float frect[4];
        frect[0] = 1.0f / float(_rect.Width());
        frect[1] = 1.0f / float(_rect.Height());

        renderer->_SetConstantVector4(flags, predefined.loc, &frect[0], 1);
      } break;
      case PREDEFINED_CONSTANT_VIEW: {
        renderer->_SetConstantMatrix4(flags, predefined.loc, &_view[eye][view], min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_INV_VIEW: {
        u16 view_eye = (view << 1) | eye;
        if (view_eye != _inv_view_cached) {
          _inv_view_cached = view_eye;
          _inv_view = _view[eye][view].Inverted();
        }
        renderer->_SetConstantMatrix4(flags, predefined.loc, &_inv_view, min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_PROJ: {
        renderer->_SetConstantMatrix4(flags, predefined.loc, &render->proj[eye][view], min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_INV_PROJ: {
        u16 view_eye = (view << 1) | eye;
        if (view_eye != _inv_proj_cached) {
          _inv_proj_cached = view_eye;
          _inv_proj = render->proj[eye][view].Inverted();
        }
        renderer->_SetConstantMatrix4(flags, predefined.loc, &_inv_proj, min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_VIEW_PROJ: {
        renderer->_SetConstantMatrix4(flags, predefined.loc, &_view_proj[eye][view], min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_INV_VIEW_PROJ: {
        u16 view_eye = (view << 1) | eye;
        if (view_eye != _inv_view_proj_cached) {
          _inv_view_proj_cached = view_eye;
          _inv_view_proj = _view_proj[eye][view];
          _inv_view_proj.InverseColOrthogonal();
        }
        renderer->_SetConstantMatrix4(flags, predefined.loc, &_inv_view_proj, min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_MODEL: {
        const float4x4& model = render->matrix_cache._cache[draw.matrix];
        renderer->_SetConstantMatrix4(flags, predefined.loc, &model, min<u32>(draw.num * MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_INV_MODEL: {
        const float4x4& inv_model = render->matrix_cache._cache[draw.matrix].Inverted();
        renderer->_SetConstantMatrix4(flags, predefined.loc, &inv_model, min<u32>(draw.num * MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_MODEL_VIEW: {
        const float4x4& model = render->matrix_cache._cache[draw.matrix];
        float4x4 model_view = _view[eye][view] * model;
        renderer->_SetConstantMatrix4(flags, predefined.loc, &model_view, min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_MODEL_VIEW_PROJ: {
        const float4x4& model = render->matrix_cache._cache[draw.matrix];
        float4x4 model_view_proj = _view_proj[eye][view] * model;
        renderer->_SetConstantMatrix4(flags, predefined.loc, &model_view_proj, min<u32>(MaxRegCount, predefined.count));
      } break;
      case PREDEFINED_CONSTANT_ALPHA_REF: {
        renderer->_SetConstantVector4(flags, predefined.loc, &_alpha_ref, 1);
      } break;
      case PREDEFINED_CONSTANT_TIME: {
        renderer->_SetConstantFloat(flags, predefined.loc, &_time, 1);
      } break;
      case PREDEFINED_CONSTANT_EYE: {
        u16 view_eye = (view << 1) | eye;
        if (view_eye != _inv_view_cached) {
          _inv_view_cached = view_eye;
          _inv_view = _view[eye][view].Inverted();
        }
        auto camera_pos = _inv_view.TranslatePart();
        renderer->_SetConstantFloat(flags, predefined.loc, &camera_pos, 3);
      } break;
      default:
        c3_log("[C3] Predefined %d not handled\n", predefined.type);
        break;
      }
    }
  }

  float4x4 _view_tmp[2][C3_MAX_VIEWS];
  float4x4 _view_proj[2][C3_MAX_VIEWS];
  float4x4* _view[2];
  Rect _rect;
  float4x4 _inv_view;
  float4x4 _inv_proj;
  float4x4 _inv_view_proj;
  float _alpha_ref;
  float _time;
  u16 _inv_view_cached;
  u16 _inv_proj_cached;
  u16 _inv_view_proj_cached;
};
