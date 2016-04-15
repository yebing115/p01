#pragma once
#include "Platform/C3Platform.h"
#include "RenderKey.h"
#include "RenderFrame.h"
#include "Pattern/Handle.h"
#include "Pattern/Singleton.h"
#include "GraphicsTypes.h"
#include "VertexFormat.h"

#define C3_CHUNK_MAGIC_TEX MAKE_FOURCC('T', 'E', 'X', ' ')
#define C3_CHUNK_MAGIC_VSH MAKE_FOURCC('V', 'S', 'H', ' ')
#define C3_CHUNK_MAGIC_FSH MAKE_FOURCC('F', 'S', 'H', ' ')
#define C3_CHUNK_MAGIC_MEX MAKE_FOURCC('M', 'E', 'X', ' ')

#define TEXTURE_COMPONENT 0x1000
#define VERTEX_BUFFER_COMPONENT 0x1001
#define INDEX_BUFFER_COMPONENT 0x1002
#define SHADER_COMPONENT 0x1003
#define PROGRAM_COMPONENT 0x1004
#define CONSTANT_COMPONENT 0x1005
#define DYNAMIC_VERTEX_BUFFER_COMPONENT 0x1006
#define DYNAMIC_INDEX_BUFFER_COMPONENT 0x1007
#define FRAME_BUFFER_COMPONENT 0x1008
#define VERTEX_DECL_COMPONENT 0x1009

#define MAX_SHADER_INPUTS 8
#define MAX_SHADER_OUTPUTS 8
#define MAX_SHADER_CONSTANTS 16

namespace ShaderInfo {

#pragma pack(push, 1)
struct Input {
  stringid name;
  stringid semantic;
  u16 data_type;
  u16 num;
  u16 normalized;
  u16 as_int;
};

struct Output {
  stringid name;
  stringid semantic;
  u16 data_type;
  u16 num;
  u32 pad;
};

struct Constant {
  stringid name;       // offset inside string table
  u8 constant_type;
  u8 num;
  u16 loc;
};

struct Header {
  u32 magic;
  u32 hash;
  u8 num_inputs;
  u8 num_outputs;
  u8 num_constants;
  u8 pad0;
  u16 string_offset;
  u16 code_offset;
  u32 code_size;
  u32 cbuffer_size;
  Input inputs[MAX_SHADER_INPUTS];
  Output outputs[MAX_SHADER_OUTPUTS];
  Constant constants[MAX_SHADER_CONSTANTS];
};
#pragma pack(pop)
}

struct MemoryRegion;
struct CommandBuffer;
struct RenderItem;
struct TextureCreate {
  u32 flags;
  u16 width;
  u16 height;
  u16 sides;
  u16 depth;
  u8 num_mips;
  u8 format;
  bool cube_map;
  const MemoryRegion* mem;
};

struct ShaderRef {
  ConstantHandle* constants;
  u32 hash;
  i16 ref_count;
  u16 num_constants;
  bool owned;
};

struct ProgramRef {
  ShaderHandle vsh;
  ShaderHandle fsh;
  i16 ref_count;
};

struct ConstantRef {
  ConstantType type;
  u16 num;
  i16 ref_count;
};

struct TextureRef {
  i16 ref_count;
  u8 bb_ratio;
  u8 format;
  bool owned;
};

struct FrameBufferRef {
  union {
    TextureHandle th[ATTACHMENT_POINT_COUNT];
    void* nwh;
  };
  bool window;

  FrameBufferRef() : window(false) {}
};

struct VertexDeclRef {
  VertexDeclHandle Find(u32 hash) {
    VertexDeclMap::const_iterator it = _vertex_decl_map.find(hash);
    if (it != _vertex_decl_map.end()) return it->second;
    return VertexDeclHandle();
  }

  void Add(VertexDeclHandle decl_handle, u32 hash) {
    _vertex_decl_map.insert(make_pair(hash, decl_handle));
  }

  typedef unordered_map<u32, VertexDeclHandle> VertexDeclMap;
  VertexDeclMap _vertex_decl_map;
};

struct VertexBuffer {
  u32 size;
  u16 stride;
  u16 flags;
};

struct IndexBuffer {
  u32 size;
  u16 flags;
};

class GraphicsRenderer {
public:
  GraphicsRenderer();
  ~GraphicsRenderer();

  bool Init(GraphicsAPI api);
  void Reset(u16 width, u16 height, u32 flags);
  void Shutdown();
  bool Ok() const { return _ok; }

  u8 PushView(const char* name = nullptr);
  u8 GetCurrentView();
  u8 GetViewCount() const { return _num_views; }
  void SetCurrentView(u8 view);
  void PopView();

  VertexBufferHandle CreateVertexBuffer(const MemoryRegion* mem, const VertexDecl& decl,
                                        u16 flags = C3_BUFFER_NONE);
  void DestroyVertexBuffer(VertexBufferHandle handle);
  IndexBufferHandle CreateIndexBuffer(const MemoryRegion* mem, u16 flags = C3_BUFFER_NONE);
  void DestroyIndexBuffer(IndexBufferHandle handle);

  VertexBufferHandle CreateDynamicVertexBuffer(u32 num, const VertexDecl& decl,
                                               u16 flags = C3_BUFFER_NONE);
  VertexBufferHandle CreateDynamicVertexBuffer(const MemoryRegion* mem, const VertexDecl& decl,
                                               u16 flags = C3_BUFFER_NONE);
  void UpdateDynamicVertexBuffer(VertexBufferHandle handle, u32 start_vertex, const MemoryRegion* mem);
  IndexBufferHandle CreateDynamicIndexBuffer(u32 num, u16 flags = C3_BUFFER_NONE);
  IndexBufferHandle CreateDynamicIndexBuffer(const MemoryRegion* mem, u16 flags = C3_BUFFER_NONE);
  void UpdateDynamicIndexBuffer(IndexBufferHandle handle, u32 start_index, const MemoryRegion* mem);

  bool CheckAvailTransientIndexBuffer(u32 num);
  bool CheckAvailTransientVertexBuffer(u32 num, const VertexDecl& decl);
  bool CheckAvailTransientBuffers(u32 num_vertices, const VertexDecl& decl, u32 num_indices);
  void AllocTransientVertexBuffer(TransientVertexBuffer* tvb, u32 num, const VertexDecl& decl);
  void AllocTransientIndexBuffer(TransientIndexBuffer* tib, u32 num);
  bool AllocTransientBuffers(TransientVertexBuffer* tvb,
                             const VertexDecl& decl, u32 num_vertices,
                             TransientIndexBuffer* tib, u32 num_indices);

  ShaderHandle CreateShader(const MemoryRegion* mem, ShaderInfo::Header* header = nullptr);
  void DestroyShader(ShaderHandle handle);
  ProgramHandle CreateProgram(ShaderHandle vsh, ShaderHandle fsh, bool destroy_shaders = true);
  void DestroyProgram(ProgramHandle handle);
  TextureHandle CreateTexture(const MemoryRegion* mem, u32 flags = C3_TEXTURE_NONE, u8 skip = 0,
                              TextureInfo* info_out = nullptr,
                              BackbufferRatio ratio = BACKBUFFER_RATIO_COUNT);
  TextureHandle CreateTexture2D(BackbufferRatio ratio, u8 mipmap_count, TextureFormat format,
                                u32 flags = C3_TEXTURE_NONE, TextureInfo* info_out = nullptr);
  TextureHandle CreateTexture2D(u16 width, u16 height, u8 mipmap_count, TextureFormat format,
                                u32 flags = C3_TEXTURE_NONE, const MemoryRegion* mem = nullptr,
                                TextureInfo* info_out = nullptr);
  void UpdateTexture2D(TextureHandle handle, u8 mip, u16 x, u16 y, u16 width, u16 height,
                       const MemoryRegion* mem, u16 pitch = UINT16_MAX);
  void ResizeTexture(TextureHandle handle, u16 width, u16 height);
  void DestroyTexture(TextureHandle handle);
  ConstantHandle CreateConstant(stringid name, ConstantType type, u16 num = 1);
  void DestroyConstant(ConstantHandle handle);

  FrameBufferHandle CreateFrameBuffer(u8 num, TextureHandle* textures, bool destroy_textures = true);
  void DestroyFrameBuffer(FrameBufferHandle handle);

  u16 SetTransform(const float4x4* mtx, u16 num = 1);
  u16 AllocTransform(float4x4*& mtx_out, u16& num_in_out);
  void SetTransform(u16 cache, u16 num = 1);

  const InstanceDataBuffer* AllocInstanceDataBuffer(u32 num, u16 stride);
  bool CheckAvailInstanceDataBuffer(u32 num, u16 stride);
#if 0
  void SetInstanceDataBuffer(const InstanceDataBuffer* idb, u32 num = UINT32_MAX);
  void SetInstanceDataBuffer(Handle handle, u32 start_vertex, u32 num);
#endif

  void SetVertexBuffer(const TransientVertexBuffer* tvb) { SetVertexBuffer(tvb, 0, UINT32_MAX); }
  void SetVertexBuffer(const TransientVertexBuffer* tvb, u32 start_vertex, u32 num_vertices);
  void SetVertexBuffer(VertexBufferHandle handle) { SetVertexBuffer(handle, 0, UINT32_MAX); }
  void SetVertexBuffer(VertexBufferHandle handle, u32 start, u32 num);
  void SetIndexBuffer(const TransientIndexBuffer* tib) { SetIndexBuffer(tib, 0, UINT32_MAX); }
  void SetIndexBuffer(const TransientIndexBuffer* tib, u32 first_index, u32 num_indices);
  void SetIndexBuffer(IndexBufferHandle handle) { SetIndexBuffer(handle, 0, UINT32_MAX); }
  void SetIndexBuffer(IndexBufferHandle handle, u32 start, u32 num);
  void SetScissor(i16 x, i16 y, i16 width, i16 height);
  void SetFrameBuffer(FrameBufferHandle handle);
  void SetConstant(ConstantHandle handle, const void* value, u16 num = 1);
  void SetTexture(u8 unit, TextureHandle texture, u32 flags = UINT32_MAX);
  void SetTexture(u8 unit, FrameBufferHandle framebuffer, int idx, u32 flags = UINT32_MAX);
  void SetViewRect(u8 view, u16 x, u16 y, u16 width, u16 height);
  void SetViewScissor(u8 view, i16 x = 0, i16 y = 0, i16 width = 0, i16 height = 0);
  void SetViewSeq(u8 view, bool enable);
  void SetViewClear(u8 view, u16 flags, u32 rgba = 0x000000ff, float depth = 1.f, u8 stencil = 0);
  void SetViewClear(u8 view, u16 flags, float depth = 1.f, u8 stencil = 0,
                    u8 attach0 = UINT8_MAX, u8 attach1 = UINT8_MAX,
                    u8 attach2 = UINT8_MAX, u8 attach3 = UINT8_MAX,
                    u8 attach4 = UINT8_MAX, u8 attach5 = UINT8_MAX,
                    u8 attach6 = UINT8_MAX, u8 attach7 = UINT8_MAX);
  void SetViewFrameBuffer(u8 view, FrameBufferHandle fbh);
  void SetViewTransform(u8 view, const float* view_matrix, const float* proj_left,
                        u8 flags = C3_VIEW_STEREO, const float* proj_right = nullptr);
  void SetViewRemap(u8 start_view, u8 num, u8* remap);
  void SetViewName(u8 view, const char* name);
  void SetPaletteColor(u8 index, u32 rgba);
  void SetPaletteColor(u8 index, float r, float g, float b, float a);
  void SetPaletteColor(u8 index, const float rgba[4]);
  void SetPaletteColor(u8 index, const Color& color);
  void SetState(u64 state, u32 rgba = 0);
  void Touch(u8 view) { Submit(view, ProgramHandle()); }
  void SetMarker(const char* marker);
  void Submit(u8 view, ProgramHandle program, i32 tag = 0);
  void Discard();
  void Frame();

  float2 GetWindowSize() const { return float2(_resolution.width, _resolution.height); }
  float GetWindowAspect() const { return float(_resolution.width) / float(_resolution.height); }

private:
  TextureHandle CreateTexture2D(BackbufferRatio ratio, u16 width, u16 height, u8 mipmap_count,
                                TextureFormat format, u32 flags, const MemoryRegion* mem, TextureInfo* info_out);
  i32 RenderOneFrame();
  void Swap();
  void FrameNoRenderWait();
  void TextureIncRef(TextureHandle handle);
  void TextureDecRef(TextureHandle handle);
  void TextureOwn(TextureHandle handle);
  void ShaderIncRef(ShaderHandle handle);
  void ShaderDecRef(ShaderHandle handle);
  void ShaderOwn(ShaderHandle handle);
  VertexDeclHandle FindVertexDecl(const VertexDecl& decl);
  u64 AllocDynamicIndexBuffer(u32 size, u16 flags);
  void FreeAllHandles(RenderFrame* frame);
  TransientIndexBuffer* CreateTransientIndexBuffer(u32 size);
  void DestroyTransientIndexBuffer(TransientIndexBuffer* tib);
  TransientVertexBuffer* CreateTransientVertexBuffer(u32 size, const VertexDecl* decl = nullptr);
  void DestroyTransientVertexBuffer(TransientVertexBuffer* tvb);

  bool _ok;
  GraphicsInterface* _gi;
  Resolution _resolution;
  u32 _frame_counter;
  RenderFrame* _frame;
  ClearQuad _clear_quad;
  VertexBuffer _vertex_buffers[C3_MAX_VERTEX_BUFFERS];
  IndexBuffer _index_buffers[C3_MAX_INDEX_BUFFERS];
  HandleAlloc<VERTEX_BUFFER_HANDLE, C3_MAX_VERTEX_BUFFERS> _vertex_buffer_handles;
  HandleAlloc<INDEX_BUFFER_HANDLE, C3_MAX_INDEX_BUFFERS>  _index_buffer_handles;
  HandleAlloc<PROGRAM_HANDLE, C3_MAX_PROGRAMS>  _program_handles;
  HandleAlloc<SHADER_HANDLE, C3_MAX_SHADERS>  _shader_handles;
  HandleAlloc<TEXTURE_HANDLE, C3_MAX_TEXTURES>  _texture_handles;
  HandleAlloc<CONSTANT_HANDLE, C3_MAX_CONSTANT_BUFER_SIZE> _constant_handles;
  HandleAlloc<FRAME_BUFFER_HANDLE, C3_MAX_FRAME_BUFFERS> _frame_buffer_handles;
  HandleAlloc<VERTEX_DECL_HANDLE, C3_MAX_VERTEX_DECLS> _vertex_decl_handles;
  Rect _rect[C3_MAX_VIEWS];
  Rect _scissor[C3_MAX_VIEWS];
  float4x4 _view[C3_MAX_VIEWS];
  float4x4 _proj[2][C3_MAX_VIEWS];
  u8 _view_flags[C3_MAX_VIEWS];
  u16 _seq[C3_MAX_VIEWS];
  bool _seq_enabled[C3_MAX_VIEWS];
  u8 _view_remap[C3_MAX_VIEWS];
  ViewClear _view_clear[C3_MAX_VIEWS];
  FrameBufferHandle _fb[C3_MAX_VIEWS];
  FrameBufferRef _frame_buffer_ref[C3_MAX_FRAME_BUFFERS];
  TextureRef _texture_ref[C3_MAX_TEXTURES];
  VertexDeclRef _decl_ref;
  u8 _num_views;
  u8 _current_view;
  typedef unordered_map<stringid, ConstantHandle> ConstantMap;
  ConstantMap _constant_map;
  typedef unordered_set<u16> ConstantSet;
  ConstantSet _constant_set;
  ConstantRef _constant_ref[C3_MAX_CONSTANTS];
  ShaderRef _shader_ref[C3_MAX_SHADERS];
  ProgramRef _program_ref[C3_MAX_PROGRAMS];
  typedef unordered_map<u32, ProgramHandle> ProgramMap;
  ProgramMap _program_map;
  float _color_palette[C3_MAX_COLOR_PALETTE][4];
  u8 _color_palette_dirty;

  friend struct RenderFrame;
  friend struct ClearQuad;
  SUPPORT_SINGLETON(GraphicsRenderer);
};
