#pragma once
#include "GraphicsTypes.h"
#include "Data/DataType.h"
//#include "Texture.h"
#include "Color.h"
#include "math/float4x4.h"
#include "ConstantBuffer.h"
#include "VertexFormat.h"

struct RenderFrame;
struct TextureInfo {
  TextureFormat format;   //!< Texture format.
  u32 storage_size;    //!< Total amount of bytes required to store texture.
  u16 width;           //!< Texture width.
  u16 height;          //!< Texture height.
  u16 depth;           //!< Texture depth.
  u8 num_mips;         //!< Number of MIP maps.
  u8 bits_per_pixel;   //!< Format bits per pixel.
  bool cube_map;          //!< Texture is cubemap.
};

struct TextureRect {
  u16 x, y, width, height;
};

struct TransientIndexBuffer {
  u8* data;            //!< Pointer to data.
  u32 size;            //!< Data size.
  u32 start_index;     //!< First index.
  IndexBufferHandle handle; //!< Index buffer handle.
};
struct TransientVertexBuffer {
  u8* data;                  //!< Pointer to data.
  u32 size;                  //!< Data size.
  u32 start_vertex;          //!< First vertex.
  u16 stride;                //!< Vertex stride.
  VertexBufferHandle handle;  //!< Vertex buffer handle.
  VertexDeclHandle decl;      //!< Vertex declaration handle.
};

struct ClearQuad {
  ClearQuad() {
    for (u32 i = 0; i < ARRAY_SIZE(program); ++i) program[i].Reset();
  }

  void Init();
  void Shutdown();

  TransientVertexBuffer* vb;
  VertexDecl decl;
  ProgramHandle program[ATTACHMENT_POINT_COUNT];
};

struct InstanceDataBuffer {
  u8* data;
  u32 size;
  u32 offset;
  u32 num;
  u16 stride;
  VertexBufferHandle handle;
};

struct Resolution {
  u16 width;
  u16 height;
  u32 flags;
  Resolution(): width(C3_RESOLUTION_DEFAULT_WIDTH), height(C3_RESOLUTION_DEFAULT_HEIGHT), flags(C3_RESET_NONE) {}
};

inline bool need_border_color(u32 flags) {
	return C3_TEXTURE_U_BORDER == (flags & C3_TEXTURE_U_BORDER) ||
    C3_TEXTURE_V_BORDER == (flags & C3_TEXTURE_V_BORDER) ||
    C3_TEXTURE_W_BORDER == (flags & C3_TEXTURE_W_BORDER);
}

class GraphicsInterface {
public:
  virtual ~GraphicsInterface() {}

  virtual bool OK() const = 0;

  static GraphicsInterface* Instance() { return __instance; }
  static bool CreateInstances(GraphicsAPI api, int major_version, int minor_version, bool need_auxiliary);
  static void ReleaseInstances();

  virtual void Init() = 0;
  virtual void Shutdown() = 0;
  virtual void CreateIndexBuffer(IndexBufferHandle handle, const MemoryRegion* mem, u16 flags) = 0;
  virtual void DestroyIndexBuffer(IndexBufferHandle handle) = 0;
  virtual void CreateVertexDecl(VertexDeclHandle handle, const VertexDecl& decl) = 0;
  virtual void DestroyVertexDecl(VertexDeclHandle handle) = 0;
  virtual void CreateVertexBuffer(VertexBufferHandle handle, const MemoryRegion* mem, VertexDeclHandle decl_handle, u16 flags) = 0;
  virtual void DestroyVertexBuffer(VertexBufferHandle handle) = 0;
  virtual void CreateDynamicIndexBuffer(IndexBufferHandle handle, u32 size, u16 flags) = 0;
  virtual void UpdateDynamicIndexBuffer(IndexBufferHandle handle, u32 offset, u32 size, const MemoryRegion* mem) = 0;
  virtual void CreateDynamicVertexBuffer(VertexBufferHandle handle, u32 size, u16 flags) = 0;
  virtual void UpdateDynamicVertexBuffer(VertexBufferHandle handle, u32 offset, u32 size, const MemoryRegion* mem) = 0;
  virtual void CreateShader(ShaderHandle handle, const MemoryRegion* mem) = 0;
  virtual void DestroyShader(ShaderHandle handle) = 0;
  virtual void CreateProgram(ProgramHandle handle, ShaderHandle vsh, ShaderHandle fsh) = 0;
  virtual void DestroyProgram(ProgramHandle handle) = 0;
  virtual void CreateTexture(TextureHandle handle, const MemoryRegion* mem, u32 flags, u8 skip) = 0;
  virtual void UpdateTextureBegin(TextureHandle handle, u8 side, u8 mip) = 0;
  virtual void UpdateTexture(TextureHandle handle, u8 side, u8 mip, const TextureRect& rect, u16 z, u16 depth, u16 pitch, const MemoryRegion* mem) = 0;
  virtual void UpdateTextureEnd() = 0;
  virtual void ResizeTexture(TextureHandle handle, u16 width, u16 height) = 0;
  virtual void DestroyTexture(TextureHandle handle) = 0;
  virtual void CreateFrameBuffer(FrameBufferHandle handle, u8 num, const TextureHandle* texture_handles) = 0;
  virtual void CreateFrameBuffer(FrameBufferHandle handle, void* nwh, u32 width, u32 height, TextureFormat depth_format) = 0;
  virtual void DestroyFrameBuffer(FrameBufferHandle handle) = 0;
  virtual void CreateConstant(ConstantHandle handle, ConstantType type, u16 num, stringid name) = 0;
  virtual void DestroyConstant(ConstantHandle handle) = 0;
  virtual void UpdateConstant(u16 loc, const void* data, u32 size) = 0;
  virtual void SetMarker(const char* marker, u32 size) = 0;
  virtual void Submit(RenderFrame* render, ClearQuad& clear_quad) = 0;
  virtual void Flip() = 0;
  virtual void SaveScreenshot(const String& path) = 0;
  void UpdateViewName(u8 view, const char* name);

protected:
  GraphicsInterface(GraphicsAPI api, int major_version, int minor_version); // create the main GI
  void UpdateConstants(ConstantBuffer* constant_buffer, u32 begin, u32 end);

  virtual void _SetConstantFloat(u8 flags, u32 loc, const void* val, u32 num) = 0;
  virtual void _SetConstantVector4(u8 flags, u32 loc, const void* val, u32 num) = 0;
  virtual void _SetConstantMatrix4(u8 flags, u32 loc, const void* val, u32 num) = 0;

  GraphicsAPI _api;
  int _major_version, _minor_version;
  char _view_names[C3_MAX_VIEWS][C3_MAX_VIEW_NAME];

private:
  static thread_local GraphicsInterface* __instance;
};
