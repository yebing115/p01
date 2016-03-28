#pragma once
#include "Platform/PlatformConfig.h"
#include "Data/DataType.h"
#include "Memory/C3Memory.h"
#include "RenderItem.h"
#include "CommandBuffer.h"
#include "GraphicsInterface.h"
#include "RenderKey.h"

class ConstantBuffer;

enum BackbufferRatio {
	BACKBUFFER_RATIO_EQUAL,
	BACKBUFFER_RATIO_HALF,
	BACKBUFFER_RATIO_QUARTER,
	BACKBUFFER_RATIO_EIGHTH,
	BACKBUFFER_RATIO_SIXTEENTH,
	BACKBUFFER_RATIO_DOUBLE,
	BACKBUFFER_RATIO_COUNT
};

struct PredefinedConstant {
  u32 loc;
  u16 count;
  u8 type;
  static stringid TypeToName(PredefinedConstantType type);
  static PredefinedConstantType NameToType(stringid name);
};

struct ViewClear {
  u8 index[8];
  float depth;
  u8 stencil;
  u16 flags;
};

struct MatrixCache {
  MatrixCache(): _num(1) { _cache[0].SetIdentity(); }
  void Reset() { _num = 1; }
  u16 Reserve(u16* num_) {
    u16 num = *num_;
    c3_assert(_num + num < C3_MAX_MATRIX_CACHE && "Matrix cache overflow.");
    num = min<u16>(num, C3_MAX_MATRIX_CACHE - _num);
    u16 first = _num;
    _num += num;
    *num_ = num;
    return first;
  }
  u16 Add(const void* mtx, u16 num) {
    if (mtx != NULL) {
      u16 first = Reserve(&num);
      memcpy(&_cache[first], mtx, sizeof(float4x4) * num);
      return first;
    }
    return 0;
  }
  float* GetBuffer(u16 cache_idx) { return _cache[cache_idx].ptr(); }
  u16 FromBuffer(const void* ptr) const { return u16((const float4x4*)ptr - _cache); }

  float4x4 _cache[C3_MAX_MATRIX_CACHE];
  u16 _num;
};

struct RectCache {
  RectCache(): _num(0) {}
  void Reset() { _num = 0; }
  u32 Add(i16 x, i16 y, i16 width, i16 height) {
    c3_assert(_num + 1 < C3_MAX_RECT_CACHE && "Rect cache overflow.");

    u32 first = _num;
    Rect& rect = _cache[_num];

    rect.left = x;
    rect.bottom = y;
    rect.right = x + width;
    rect.top = y + height;

    _num++;
    return first;
  }

  Rect _cache[C3_MAX_RECT_CACHE];
  u32 _num;
};

struct RenderFrame {
  ConstantBuffer* constant_buffer;
  u32 constant_begin;
  u32 constant_end;
  u32 constant_max;
  float color_palette[C3_MAX_COLOR_PALETTE][4];
  bool color_palette_dirty;
  ClearQuad clear_quad;

  RenderItem render_items[C3_MAX_DRAW_CALLS];
  u16 render_item_count;
  RenderItem current;

  MatrixCache matrix_cache;
  RectCache rect_cache;

  u8 view_remap[C3_MAX_VIEWS];
  Handle fb[C3_MAX_VIEWS];
  SortKey sort_key;
  u64 sort_keys[C3_MAX_DRAW_CALLS];
  u16 sort_values[C3_MAX_DRAW_CALLS];
  Rect rect[C3_MAX_VIEWS];
  Rect scissor[C3_MAX_VIEWS];
  float4x4 _view[C3_MAX_VIEWS];
  float4x4 proj[2][C3_MAX_VIEWS];
  u8 view_flags[C3_MAX_VIEWS];
  ViewClear view_clear[C3_MAX_VIEWS];

  u32 ib_offset;
  u32 vb_offset;
  TransientIndexBuffer* transient_ib;
  TransientVertexBuffer* transient_vb;

  Resolution resolution;

  CommandBuffer pre_cmd_buffer;
  CommandBuffer post_cmd_buffer;
  bool discard;

  Handle free_index_buffer_handle[C3_MAX_INDEX_BUFFERS];
  Handle free_vertex_decl_handle[C3_MAX_VERTEX_DECLS];
  Handle free_vertex_buffer_handle[C3_MAX_VERTEX_BUFFERS];
  Handle free_shader_handle[C3_MAX_SHADERS];
  Handle free_program_handle[C3_MAX_PROGRAMS];
  Handle free_texture_handle[C3_MAX_TEXTURES];
  Handle free_frame_buffer_handle[C3_MAX_FRAME_BUFFERS];
  Handle free_constant_handle[C3_MAX_CONSTANTS];
  u16 num_free_index_buffer_handles;
  u16 num_free_vertex_decl_handles;
  u16 num_free_vertex_buffer_handles;
  u16 num_free_shader_handles;
  u16 num_free_program_handles;
  u16 num_free_texture_handles;
  u16 num_free_frame_buffer_handles;
  u16 num_free_constant_handles;
  u16 num_free_window_handles;

  RenderFrame();
  ~RenderFrame();

  void Create();
  void Destroy();
  void Start();
  void Finish();
  void Clear();
  void Reset();
  void Sort();
  void Discard();
  void SetMarker(const char* marker);
  void SetState(u64 state, u32 rgba);
  void SetVertexBuffer(const TransientVertexBuffer* tvb, u32 start_vertex, u32 num_vertices);
  void SetVertexBuffer(Handle handle, u32 start_vertex, u32 num_vertices);
  void SetIndexBuffer(const TransientIndexBuffer* tib, u32 first_index, u32 num_indices);
  void SetIndexBuffer(Handle handle, u32 start_index, u32 num_indices);
  void SetTexture(u8 unit, Handle handle, u32 flags);
  void SetConstant(ConstantType type, Handle handle, const void* value, u16 num);
  void SetScissor(i16 x, i16 y, i16 width, i16 height);
  u16 SetTransform(const void* m, u16 num);
  void SetTransform(u16 cache, u16 num);
  u16 AllocTransform(float4x4*& m_out, u16& num_in_out);
  bool CheckAvailTransientIndexBuffer(u32 num);
  u32 AllocTransientIndexBuffer(u32& num_in_out);
  bool CheckAvailTransientVertexBuffer(u32 num, u16 stride);
  u32 AllocTransientVertexBuffer(u32& num_in_out, u16 stride);
  void SetInstanceDataBuffer(const InstanceDataBuffer* idb, u32 num);
  void SetInstanceDataBuffer(Handle handle, u32 start_vertex, u32 num, u16 stride);
  void SetViewName(u8 view, const char* name);
  void Submit(u8 view, Handle program, i32 tag);

  CommandBuffer& GetCommandBuffer(CommandBuffer::CommandType type) {
    auto& cmd = type < CommandBuffer::END ? pre_cmd_buffer : post_cmd_buffer;
    cmd.Write((u8)type);
    return cmd;
  }

  void Free(Handle handle) { free_frame_buffer_handle[num_free_frame_buffer_handles++] = handle; }
  void ResetFreeHandles();
};
