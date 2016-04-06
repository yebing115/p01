#pragma once
#include "Graphics/GraphicsInterface.h"
#include "Graphics/ConstantBuffer.h"
#include "Graphics/ViewState.h"
#include "Memory/C3Memory.h"
#include "dx_header.h"

#ifdef REMOTERY_SUPPORT
#define GPU_PROFILE_BLOCK(block) rmt_ScopedD3D11Sample(block)
#define BEGIN_GPU_PROFILE_BLOCK(block) rmt_BeginD3D11Sample(block)
#define BEGIN_GPU_PROFILE_BLOCK_DYNAMIC(block_str) rmt_BeginD3D11SampleDynamic(block_str)
#define END_GPU_PROFILE_BLOCK() rmt_EndD3D11Sample()
#else
#define GPU_PROFILE_BLOCK(block)
#define BEGIN_GPU_PROFILE_BLOCK(block)
#define BEGIN_GPU_PROFILE_BLOCK_DYNAMIC(block_str)
#define END_GPU_PROFILE_BLOCK()
#endif

#define _DX_CHECK(call) \
        { \
	  HRESULT __hr__ = call; \
		if (FAILED(__hr__)) c3_log("%s:%d " #call " FAILED 0x%08x\n", __FILE__, __LINE__, (u32)__hr__); \
        }

#ifdef _DEBUG
#define DX_CHECK(call) _DX_CHECK(call)
#else
#define DX_CHECK(call) call
#endif

#define DX_RELEASE(_ptr) \
  if (_ptr) { \
    (_ptr)->Release(); \
    _ptr = NULL; \
  }

#ifndef D3DCOLOR_ARGB
#define D3DCOLOR_ARGB(a, r, g, b) ((DWORD)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))
#endif // D3DCOLOR_ARGB

#ifndef D3DCOLOR_RGBA
#define D3DCOLOR_RGBA(r, g, b, a) D3DCOLOR_ARGB(a, r, g, b)
#endif // D3DCOLOR_RGBA

#define C3_D3D11_BLEND_STATE_MASK (0 \
			| C3_STATE_BLEND_MASK \
			| C3_STATE_BLEND_EQUATION_MASK \
			| C3_STATE_BLEND_INDEPENDENT \
			| C3_STATE_ALPHA_WRITE \
			| C3_STATE_RGB_WRITE \
			)

#define C3_D3D11_DEPTH_STENCIL_MASK (0 \
			| C3_STATE_DEPTH_WRITE \
			| C3_STATE_DEPTH_TEST_MASK \
			)

#define C3_D3D11_MAX_STAGING_TEXTURES 12

struct BufferD3D11 {
  BufferD3D11()
    : _ptr(NULL)
    , _srv(NULL)
    , _uav(NULL)
    , _flags(C3_BUFFER_NONE)
    , _dynamic(false) {}

  void Create(u32 size, void* data, u16 flags, u16 stride = 0, bool vertex = false);
  void Update(u32 offset, u32 size, void* data, bool discard = false);

  void Destroy() {
    if (NULL != _ptr) {
      _ptr->Release();
      _dynamic = false;
    }

    _srv->Release();
    _uav->Release();
  }

  ID3D11Buffer* _ptr;
  ID3D11ShaderResourceView*  _srv;
  ID3D11UnorderedAccessView* _uav;
  u32 _size;
  u16 _flags;
  bool _dynamic;
};

typedef BufferD3D11 IndexBufferD3D11;

struct VertexBufferD3D11 : public BufferD3D11 {
  VertexBufferD3D11()
    : BufferD3D11() {}

  void Create(u32 size, void* data, VertexDeclHandle decl_handle, u16 flags);

  VertexDeclHandle _decl;
};

struct ShaderD3D11 {
  ShaderD3D11()
  : _ptr(NULL)
  , _code(NULL)
  , _buffer(NULL)
  , _constant_buffer(NULL)
  , _hash(0)
  , _num_uniforms(0)
  , _num_predefined(0)
  , _has_depth_op(false) {}

  void Create(const MemoryRegion* mem);
  DWORD* GetShaderCode(u8 fragment_bit, const MemoryRegion* mem);

  void Destroy() {
    if (NULL != _constant_buffer) {
      ConstantBuffer::Destroy(_constant_buffer);
      _constant_buffer = NULL;
    }

    _num_predefined = 0;

    if (NULL != _buffer) {
      _buffer->Release();
    }

    _ptr->Release();

    if (NULL != _code) {
      mem_free(_code);
      _code = NULL;
      _hash = 0;
    }
  }

  union {
    ID3D11ComputeShader* _compute_shader;
    ID3D11PixelShader*   _pixel_shader;
    ID3D11VertexShader*  _vertex_shader;
    IUnknown*            _ptr;
  };
  const MemoryRegion* _code;
  ID3D11Buffer* _buffer;
  ConstantBuffer* _constant_buffer;
  UINT _byte_width;

  PredefinedConstant _predefined[PREDEFINED_CONSTANT_COUNT];
  u16 _attr_mask[VERTEX_ATTR_COUNT + INSTANCE_ATTR_COUNT];

  u32 _hash;

  u16 _num_uniforms;
  u8 _num_predefined;
  bool _has_depth_op;
};

struct ProgramD3D11 {
  ProgramD3D11(): _vsh(NULL), _fsh(NULL) {}

  void Create(const ShaderD3D11* vsh, const ShaderD3D11* fsh) {
    c3_assert(vsh->_ptr && "Vertex shader doesn't exist.");
    _vsh = vsh;
    memcpy(&_predefined[0], _vsh->_predefined, _vsh->_num_predefined*sizeof(PredefinedConstant));
    _num_predefined = _vsh->_num_predefined;

    if (NULL != fsh) {
      c3_assert(fsh->_ptr && "Fragment shader doesn't exist.");
      _fsh = fsh;
      memcpy(&_predefined[_num_predefined], _fsh->_predefined, _fsh->_num_predefined*sizeof(PredefinedConstant));
      _num_predefined += _fsh->_num_predefined;
    }
  }

  void Destroy() {
    _num_predefined = 0;
    _vsh = NULL;
    _fsh = NULL;
  }

  const ShaderD3D11* _vsh;
  const ShaderD3D11* _fsh;

  PredefinedConstant _predefined[PREDEFINED_CONSTANT_COUNT * 2];
  u8 _num_predefined;
};

struct TextureD3D11 {
  enum Enum {
    Texture2D,
    Texture3D,
    TextureCube,
  };

  TextureD3D11(): _ptr(NULL), _resolved_texture2d(NULL), _srv(NULL), _uav(NULL), _num_mips(0) {}

  void Create(const MemoryRegion* mem, u32 flags, u8 skip);
  void Destroy();
  void Update(u8 side, u8 mip, const TextureRect& rect, u16 z, u16 depth, u16 pitch, const MemoryRegion* mem);
  void Commit(u8 stage, u32 flags, const float palette[][4]);
  void Resolve();

  union {
    ID3D11Resource* _ptr;
    ID3D11Texture2D* _texture2d;
    ID3D11Texture3D* _texture3d;
  };
  ID3D11Texture2D* _resolved_texture2d;

  ID3D11ShaderResourceView* _srv;
  ID3D11UnorderedAccessView* _uav;
  u32 _flags;
  u32 _width;
  u32 _height;
  u32 _depth;
  u8 _type;
  u8 _requested_format;
  u8 _texture_format;
  u8 _num_mips;
};

struct FrameBufferD3D11 {
  FrameBufferD3D11()
  : _dsv(NULL)
  , _swap_chain(NULL)
  , _width(0)
  , _height(0)
  , _dense_idx(UINT16_MAX)
  , _num(0)
  , _num_th(0) {}

  void Create(u8 num, const TextureHandle* handles);
  void Create(u16 dense_idx, void* nwh, u32 width, u32 height, TextureFormat depth_format);
  u16 Destroy();
  void PreReset(bool force = false);
  void PostReset();
  void Resolve();
  void Clear(const ViewClear& view_clear, const float palette[][4]);

  ID3D11RenderTargetView* _rtv[ATTACHMENT_POINT_COUNT - 1];
  ID3D11ShaderResourceView* _srv[ATTACHMENT_POINT_COUNT - 1];
  ID3D11DepthStencilView* _dsv;
  IDXGISwapChain* _swap_chain;
  u32 _width;
  u32 _height;
  u16 _dense_idx;
  u8 _num;
  u8 _num_th;
  TextureHandle _th[ATTACHMENT_POINT_COUNT];
};

struct TextureStage {
  TextureStage() { Clear(); }

  void Clear() {
    memset(_srv, 0, sizeof(_srv));
    memset(_sampler, 0, sizeof(_sampler));
  }

  ID3D11ShaderResourceView* _srv[C3_MAX_TEXTURE_SAMPLERS];
  ID3D11SamplerState* _sampler[C3_MAX_TEXTURE_SAMPLERS];
};

template <typename Ty>
class StateCacheT {
public:
  void Add(u64 _key, Ty* _value) {
    Invalidate(_key);
    _hash_map.insert(make_pair(_key, _value));
  }

  Ty* Find(u64 _key) {
    typename HashMap::iterator it = _hash_map.find(_key);
    if (it != _hash_map.end()) {
      return it->second;
    }

    return NULL;
  }

  void Invalidate(u64 _key) {
    typename HashMap::iterator it = _hash_map.find(_key);
    if (it != _hash_map.end()) {
      it->second->Release();
      _hash_map.erase(it);
    }
  }

  void Invalidate() {
    for (typename HashMap::iterator it = _hash_map.begin(), itEnd = _hash_map.end(); it != itEnd; ++it) {
      it->second->Release();
    }

    _hash_map.clear();
  }

  u32 GetCount() const {
    return u32(_hash_map.size());
  }

private:
  typedef unordered_map<u64, Ty*> HashMap;
  HashMap _hash_map;
};

struct StagingTextureD3D11 {
  ID3D11Texture2D* _staging_textures;
  D3D11_TEXTURE2D_DESC _desc;
  void* _mapped_ptr;
};

class GraphicsInterfaceD3D11 : public GraphicsInterface {
public:
  GraphicsInterfaceD3D11(GraphicsAPI api, int major_version, int minor_version);
  ~GraphicsInterfaceD3D11();

  // init
  bool OK() const override { return true; }

  void Init() override;
  void Shutdown() override;
  void CreateIndexBuffer(IndexBufferHandle handle, const MemoryRegion* mem, u16 flags) override;
  void DestroyIndexBuffer(IndexBufferHandle handle) override;
  void CreateVertexDecl(VertexDeclHandle handle, const VertexDecl& decl) override;
  void DestroyVertexDecl(VertexDeclHandle handle) override;
  void CreateVertexBuffer(VertexBufferHandle handle, const MemoryRegion* mem,
                          VertexDeclHandle decl_handle, u16 flags) override;
  void DestroyVertexBuffer(VertexBufferHandle handle) override;
  void CreateDynamicIndexBuffer(IndexBufferHandle handle, u32 size, u16 flags) override;
  void UpdateDynamicIndexBuffer(IndexBufferHandle handle, u32 offset, u32 size, const MemoryRegion* mem) override;
  void CreateDynamicVertexBuffer(VertexBufferHandle handle, u32 size, u16 flags) override;
  void UpdateDynamicVertexBuffer(VertexBufferHandle handle, u32 offset, u32 size, const MemoryRegion* mem) override;
  void CreateShader(ShaderHandle handle, const MemoryRegion* mem) override;
  void DestroyShader(ShaderHandle handle) override;
  void CreateProgram(ProgramHandle handle, ShaderHandle vsh, ShaderHandle fsh) override;
  void DestroyProgram(ProgramHandle handle) override;
  void CreateTexture(TextureHandle handle, const MemoryRegion* mem, u32 flags, u8 skip) override;
  void UpdateTextureBegin(TextureHandle handle, u8 side, u8 mip) override;
  void UpdateTexture(TextureHandle handle, u8 side, u8 mip, const TextureRect& rect, u16 z, u16 depth, u16 pitch, const MemoryRegion* mem) override;
  void UpdateTextureEnd() override;
  void ResizeTexture(TextureHandle handle, u16 width, u16 height) override;
  void DestroyTexture(TextureHandle handle) override;
  void CreateFrameBuffer(FrameBufferHandle handle, u8 num, const TextureHandle* texture_handles) override;
  void CreateFrameBuffer(FrameBufferHandle handle, void* nwh, u32 width, u32 height, TextureFormat depth_format) override;
  void DestroyFrameBuffer(FrameBufferHandle handle) override;
  void CreateConstant(ConstantHandle handle, ConstantType type, u16 num, stringid name) override;
  void DestroyConstant(ConstantHandle handle) override;
  void UpdateConstant(u16 loc, const void* data, u32 size) override;
  void SetMarker(const char* marker, u32 size) override;
  void Submit(RenderFrame* render, ClearQuad& clear_quad) override;
  void Flip() override;
  void SaveScreenshot(const String& path) override;

protected:
  void _SetConstantFloat(u8 flags, u32 loc, const void* val, u32 num) override;
  void _SetConstantVector4(u8 flags, u32 loc, const void* val, u32 num) override;
  void _SetConstantMatrix4(u8 flags, u32 loc, const void* val, u32 num) override;

private:
  ID3D11SamplerState* GetSamplerState(u32 flags, const float rgba[4]);
  void UpdateMsaa();
  void UpdateResolution(const Resolution& r);
  void PreReset();
  void PostReset();
  void InvalidateCache();
  void _SetConstant(u8 flags, u32 loc, const void* val, u32 num);
  void SetFrameBuffer(FrameBufferHandle fbh, bool msaa = true);
  void Clear(const ViewClear& view_clear, const float palette[][4]);
  void Clear(ClearQuad& clear_quad, const Rect& rect, const ViewClear& clear, const float palette[][4]);
  void SetInputLayout(const VertexDecl& vertex_decl, const ProgramD3D11& program, u16 num_instance_data);
  void SetBlendState(u64 state, u32 rgba = 0);
  void SetDepthStencilState(u64 state, u64 stencil = 0);
  void SetRasterizerState(u64 state, bool wireframe = false, bool scissor = false);
  void SetDebugWireframe(bool wireframe);
  void CommitTextureStage();
  void Commit(ConstantBuffer& constant_buffer);
  void CommitShaderConstants();
  void InvalidateTextureStage();
  
  bool _initialized;

  u16 _lost;
  u16 _num_windows;
  FrameBufferHandle _windows[C3_MAX_FRAME_BUFFERS];

  ID3D11RenderTargetView* _back_buffer_color;
  ID3D11DepthStencilView* _back_buffer_depth_stencil;
  ID3D11RenderTargetView* _current_color;
  ID3D11DepthStencilView* _current_depth_stencil;

  D3D_DRIVER_TYPE _driver_type;
  D3D_FEATURE_LEVEL _feature_level;
  IDXGIAdapter* _adapter;
  DXGI_ADAPTER_DESC _adapter_desc;
  IDXGIFactory* _factory;
  IDXGISwapChain* _swap_chain;
  ID3D11Device* _device;
  ID3D11DeviceContext* _context;
  DXGI_SWAP_CHAIN_DESC _scd;

  FrameBufferHandle _fbh;
  Resolution _resolution;
  bool _wireframe;
  bool _rt_msaa;

  u8* _vs_scratch;
  u8* _fs_scratch;
  u32 _vs_changes;
  u32 _fs_changes;

  IndexBufferD3D11 _index_buffers[C3_MAX_INDEX_BUFFERS];
  VertexBufferD3D11 _vertex_buffers[C3_MAX_VERTEX_BUFFERS];
  ShaderD3D11 _shaders[C3_MAX_SHADERS];
  ProgramD3D11 _programs[C3_MAX_PROGRAMS];
  TextureD3D11 _textures[C3_MAX_TEXTURES];
  VertexDecl _vertex_decls[C3_MAX_VERTEX_DECLS];
  FrameBufferD3D11 _frame_buffers[C3_MAX_FRAME_BUFFERS];
  void* _uniforms[C3_MAX_CONSTANTS];
  float4x4 _predefined_uniforms[PREDEFINED_CONSTANT_COUNT];
  ConstantRegistry _uniform_reg;
  ViewState _view_state;

  TextureStage _texture_stage;
  ProgramD3D11* _current_program;

  StateCacheT<ID3D11BlendState> _blend_state_cache;
  StateCacheT<ID3D11DepthStencilState> _depth_stencil_state_cache;
  StateCacheT<ID3D11InputLayout> _input_layout_cache;
  StateCacheT<ID3D11RasterizerState> _rasterizer_state_cache;
  StateCacheT<ID3D11SamplerState> _sampler_state_cache;
  //StateCacheT<IUnknown*> m_srvUavLru;

  u32 _max_anisotropy;
  u32 _max_anisotropy_default;

  ID3DUserDefinedAnnotation* _user_defined_annotation;

  friend struct ViewState;
  friend struct BufferD3D11;
  friend struct VertexBufferD3D11;
  friend struct TextureD3D11;
  friend struct FrameBufferD3D11;
  friend struct ShaderD3D11;
  friend struct ProgramD3D11;
};
