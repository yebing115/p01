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
  Handle handle; //!< Index buffer handle.
};
struct TransientVertexBuffer {
  u8* data;                  //!< Pointer to data.
  u32 size;                  //!< Data size.
  u32 start_vertex;          //!< First vertex.
  u16 stride;                //!< Vertex stride.
  Handle handle;  //!< Vertex buffer handle.
  Handle decl;      //!< Vertex declaration handle.
};

struct ClearQuad {
  ClearQuad() {
    for (u32 i = 0; i < ARRAY_SIZE(program); ++i) program[i].Reset();
  }

  void Init();
  void Shutdown();

  TransientVertexBuffer* vb;
  VertexDecl decl;
  Handle program[ATTACHMENT_POINT_COUNT];
};

struct InstanceDataBuffer {
  u8* data;
  u32 size;
  u32 offset;
  u32 num;
  u16 stride;
  Handle handle;
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
  virtual void CreateIndexBuffer(Handle handle, MemoryBlock* mem, u16 flags) = 0;
  virtual void DestroyIndexBuffer(Handle handle) = 0;
  virtual void CreateVertexDecl(Handle handle, const VertexDecl& decl) = 0;
  virtual void DestroyVertexDecl(Handle handle) = 0;
  virtual void CreateVertexBuffer(Handle handle, MemoryBlock* mem, Handle decl_handle, u16 flags) = 0;
  virtual void DestroyVertexBuffer(Handle handle) = 0;
  virtual void CreateDynamicIndexBuffer(Handle handle, u32 size, u16 flags) = 0;
  virtual void UpdateDynamicIndexBuffer(Handle handle, u32 offset, u32 size, MemoryBlock* mem) = 0;
  virtual void DestroyDynamicIndexBuffer(Handle handle) = 0;
  virtual void CreateDynamicVertexBuffer(Handle handle, u32 size, u16 flags) = 0;
  virtual void UpdateDynamicVertexBuffer(Handle handle, u32 offset, u32 size, MemoryBlock* mem) = 0;
  virtual void DestroyDynamicVertexBuffer(Handle handle) = 0;
  virtual void CreateShader(Handle handle, MemoryBlock* mem) = 0;
  virtual void DestroyShader(Handle handle) = 0;
  virtual void CreateProgram(Handle handle, Handle vsh, Handle fsh) = 0;
  virtual void DestroyProgram(Handle handle) = 0;
  virtual void CreateTexture(Handle handle, MemoryBlock* mem, u32 flags, u8 skip) = 0;
  virtual void UpdateTextureBegin(Handle handle, u8 side, u8 mip) = 0;
  virtual void UpdateTexture(Handle handle, u8 side, u8 mip, const TextureRect& rect, u16 z, u16 depth, u16 pitch, const MemoryBlock* mem) = 0;
  virtual void UpdateTextureEnd() = 0;
  virtual void ResizeTexture(Handle handle, u16 width, u16 height) = 0;
  virtual void DestroyTexture(Handle handle) = 0;
  virtual void CreateFrameBuffer(Handle handle, u8 num, const Handle* texture_handles) = 0;
  virtual void CreateFrameBuffer(Handle handle, void* nwh, u32 width, u32 height, TextureFormat depth_format) = 0;
  virtual void DestroyFrameBuffer(Handle handle) = 0;
  virtual void CreateConstant(Handle handle, ConstantType type, u16 num, stringid name) = 0;
  virtual void DestroyConstant(Handle handle) = 0;
  virtual void UpdateViewName(u8 id, const char* name) = 0;
  virtual void UpdateConstant(u16 loc, const void* data, u32 size) = 0;
  virtual void SetMarker(const char* marker, u32 size) = 0;
  virtual void Submit(RenderFrame* render, ClearQuad& clear_quad) = 0;
  virtual void Flip() = 0;
  virtual void SaveScreenshot(const String& path) = 0;
  void UpdateViewName(u8 view, const char* name, int name_len);

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
