#include "C3PCH.h"
#include "GraphicsRenderer.h"
#include "ConstantBuffer.h"
#include "Image/ImageUtils.h"
#include "GraphicsTypes.h"
#include "RenderFrame.h"
#include "Data/Blob.h"
#include <thread>

DEFINE_SINGLETON_INSTANCE(GraphicsRenderer);

static void calc_texture_size(TextureInfo& info_out, u16 width_, u16 height_, u16 depth_, bool cube_map, u8 num_mips, TextureFormat format);
static void get_texture_size_from_ratio(BackbufferRatio ratio, u16& width_out, u16& height_out);

GraphicsRenderer::GraphicsRenderer(): _ok(false), _gi(nullptr), _frame_counter(0) {
  _frame = new RenderFrame;
  _color_palette_dirty = 0;
  _num_views = 0;
  memset(_view_flags, C3_VIEW_NONE, sizeof(_view_flags));
  memset(_seq_enabled, 0, sizeof(_seq_enabled));
  memset(_rect, 0, sizeof(_rect));
  memset(_scissor, 0, sizeof(_scissor));
  for (u8 i = 0; i < C3_MAX_VIEWS; ++i) {
    _view_remap[i] = i;
    _view[i].SetIdentity();
    _proj[0][i].SetIdentity();
    _proj[1][i].SetIdentity();
  }
}

GraphicsRenderer::~GraphicsRenderer() {}

bool GraphicsRenderer::Init(GraphicsAPI api) {
  _ok = false;
  c3_log("renderer size: %u, frame size: %u\n", sizeof(GraphicsRenderer), sizeof(RenderFrame));
  init_builtin_vertex_decls(api);

  _frame->Create();

  _ok = GraphicsInterface::CreateInstances(D3D11_WIN_API, 11, 0, false);

  if (!_ok) return false;

  _gi = GraphicsInterface::Instance();
  _gi->Init();
  _clear_quad.Init();

  _frame->transient_vb = CreateTransientVertexBuffer(C3_TRANSIENT_VERTEX_BUFFER_SIZE);
  _frame->transient_ib = CreateTransientIndexBuffer(C3_TRANSIENT_INDEX_BUFFER_SIZE);
  Frame();

  Reset(C3_RESOLUTION_DEFAULT_WIDTH, C3_RESOLUTION_DEFAULT_HEIGHT, C3_RESOLUTION_DEFAULT_FLAGS);
  return true;
}

void GraphicsRenderer::Reset(u16 width, u16 height, u32 flags) {
  _resolution.width = width;
  _resolution.height = height;
  _resolution.flags = flags;

  for (u32 i = 0, num = _texture_handles.GetUsed(); i < num; ++i) {
    TextureHandle texture_handle = _texture_handles.GetHandleAt(i);
    const TextureRef& texture = _texture_ref[texture_handle.idx];
    if (texture.bb_ratio != BACKBUFFER_RATIO_COUNT) {
      ResizeTexture(texture_handle, width, height);
    }
  }
}

void GraphicsRenderer::Shutdown() {
  _frame->Destroy();
}

u8 GraphicsRenderer::PushView(const char* name) {
  _current_view = _num_views++;
  if (name) SetViewName(_current_view, name);
  return _current_view;
}

u8 GraphicsRenderer::GetCurrentView() { return _current_view; }
void GraphicsRenderer::SetCurrentView(u8 view) { _current_view = view; }

void GraphicsRenderer::PopView() { --_current_view; }

VertexBufferHandle GraphicsRenderer::CreateVertexBuffer(const MemoryRegion* mem, const VertexDecl& decl, u16 flags) {
  VertexBufferHandle handle = _vertex_buffer_handles.Alloc();
  c3_assert(handle && "Failed to allocate vertex buffer handle.");
  VertexDeclHandle decl_handle = FindVertexDecl(decl);
  auto& vb = _vertex_buffers[handle.idx];
  vb.stride = decl.stride;
  vb.size = mem->size;
  vb.flags = flags;
  _gi->CreateVertexBuffer(handle, mem, decl_handle, flags);
  return handle;
}

void GraphicsRenderer::DestroyVertexBuffer(VertexBufferHandle handle) {
  if (!handle) return;
  _gi->DestroyVertexBuffer(handle);
  _vertex_buffer_handles.Free(handle);
}

IndexBufferHandle GraphicsRenderer::CreateIndexBuffer(const MemoryRegion* mem, u16 flags) {
  IndexBufferHandle handle = _index_buffer_handles.Alloc();
  auto& ib = _index_buffers[handle.idx];
  ib.size = mem->size;
  ib.flags = flags;
  _gi->CreateIndexBuffer(handle, mem, flags);
  return handle;
}

void GraphicsRenderer::DestroyIndexBuffer(IndexBufferHandle handle) {
  if (!handle) return;
  _gi->DestroyIndexBuffer(handle);
  _index_buffer_handles.Free(handle);
}

VertexBufferHandle GraphicsRenderer::CreateDynamicVertexBuffer(u32 num, const VertexDecl& decl, u16 flags) {
  VertexBufferHandle handle = _vertex_buffer_handles.Alloc();
  c3_assert(handle && "Failed to allocate dynamic vertex buffer handle.");
  VertexDeclHandle decl_handle = FindVertexDecl(decl);
  auto& vb = _vertex_buffers[handle.idx];
  vb.stride = decl.stride;
  vb.size = num * decl.stride;
  vb.flags = flags;

  _gi->CreateDynamicVertexBuffer(handle, decl.stride * num, flags);
  return handle;
}

VertexBufferHandle GraphicsRenderer::CreateDynamicVertexBuffer(const MemoryRegion* mem, const VertexDecl& decl, u16 flags) {
  u32 num_vertices = mem->size / decl.stride;
  if (num_vertices > UINT16_MAX) c3_log("Num vertices exceeds maximum (num %d, max %d).\n", num_vertices, UINT16_MAX);
  VertexBufferHandle handle = CreateDynamicVertexBuffer(u16(num_vertices), decl, flags);
  if (handle) UpdateDynamicVertexBuffer(handle, 0, mem);
  return handle;
}

void GraphicsRenderer::UpdateDynamicVertexBuffer(VertexBufferHandle handle, u32 start_vertex, const MemoryRegion* mem) {
  auto& vb = _vertex_buffers[handle.idx];
  u32 offset = start_vertex * vb.stride;
  if (offset >= vb.size || offset + mem->size > vb.size) {
    c3_log("[C3] Dynamic vertex buffer update overflow.\n");
    return;
  }
  u32 size = min<u32>(vb.size - offset, mem->size);
  if (size < mem->size) {
    c3_log("[C3] Truncating dynamic vertex buffer update (size %d, mem size %d).\n", vb.size, mem->size);
  }
  _gi->UpdateDynamicVertexBuffer(handle, offset, size, mem);
}

IndexBufferHandle GraphicsRenderer::CreateDynamicIndexBuffer(u32 num, u16 flags) {
  IndexBufferHandle handle = _index_buffer_handles.Alloc();
  const u32 index_size = 0 == (flags & C3_BUFFER_INDEX32) ? 2 : 4;
  u32 size = num * index_size;
  if (!handle) {
    c3_log("[C3] Failed to allocate dynamic index buffer handle.\n");
    return handle;
  }
  auto& ib = _index_buffers[handle.idx];
  ib.size = size;
  ib.flags = flags;
  _gi->CreateDynamicIndexBuffer(handle, size, flags);
  return handle;
}

IndexBufferHandle GraphicsRenderer::CreateDynamicIndexBuffer(const MemoryRegion* mem, u16 flags) {
  const u32 index_size = (flags & C3_BUFFER_INDEX32) ? 4 : 2;
  IndexBufferHandle handle = CreateDynamicIndexBuffer(mem->size / index_size, flags);
  if (handle) UpdateDynamicIndexBuffer(handle, 0, mem);
  return handle;
}

void GraphicsRenderer::UpdateDynamicIndexBuffer(IndexBufferHandle handle, u32 start_index, const MemoryRegion* mem) {
  IndexBuffer& ib = _index_buffers[handle.idx];
  const u32 index_size = (ib.flags & C3_BUFFER_INDEX32) ? 4 : 2;
  u32 offset = start_index * index_size;
  if (offset >= ib.size) {
    c3_log("[C3] Update dynamic index buffer, start_index too large (size %d, offset %d).\n", ib.size, offset);
    return;
  }
  u32 size = min(offset + mem->size, ib.size) - offset;
  if (size < mem->size) c3_log("Truncating dynamic index buffer update (size %d, mem size %d).\n", size, mem->size);
  _gi->UpdateDynamicIndexBuffer(handle, offset, size, mem);
}

bool GraphicsRenderer::CheckAvailTransientIndexBuffer(u32 num) {
  return _frame->CheckAvailTransientIndexBuffer(num);
}

bool GraphicsRenderer::CheckAvailTransientVertexBuffer(u32 num, const VertexDecl& decl) {
  return _frame->CheckAvailTransientVertexBuffer(num, decl.stride);
}

bool GraphicsRenderer::CheckAvailTransientBuffers(u32 num_vertices, const VertexDecl& decl, u32 num_indices) {
  return _frame->CheckAvailTransientIndexBuffer(num_indices) &&
    _frame->CheckAvailTransientVertexBuffer(num_vertices, decl.stride);
}

void GraphicsRenderer::AllocTransientVertexBuffer(TransientVertexBuffer* tvb_out, u32 num, const VertexDecl& decl) {
  TransientVertexBuffer& dvb = *_frame->transient_vb;
  
  VertexDeclHandle decl_handle = _decl_ref.Find(decl.hash);
  if (!decl_handle) {
    VertexDeclHandle temp = _vertex_decl_handles.Alloc();
    decl_handle = temp;
    _gi->CreateVertexDecl(decl_handle, decl);
    _decl_ref.Add(decl_handle, decl.hash);
  }

  u32 offset = _frame->AllocTransientVertexBuffer(num, decl.stride);

  tvb_out->data = &dvb.data[offset];
  tvb_out->size = num * decl.stride;
  tvb_out->start_vertex = offset / decl.stride;
  tvb_out->stride = decl.stride;
  tvb_out->handle = dvb.handle;
  tvb_out->decl = decl_handle;
}

void GraphicsRenderer::AllocTransientIndexBuffer(TransientIndexBuffer* tib_out, u32 num) {
  u32 offset = _frame->AllocTransientIndexBuffer(num);
  TransientIndexBuffer& tib = *_frame->transient_ib;

  tib_out->data = &tib.data[offset];
  tib_out->size = num * 2;
  tib_out->handle = tib.handle;
  tib_out->start_index = offset / 2;
}

bool GraphicsRenderer::AllocTransientBuffers(TransientVertexBuffer* tvb, const VertexDecl& decl, u32 num_vertices, TransientIndexBuffer* tib, u32 num_indices) {
  if (CheckAvailTransientBuffers(num_vertices, decl, num_indices)) {
    AllocTransientVertexBuffer(tvb, num_vertices, decl);
    AllocTransientIndexBuffer(tib, num_indices);
    return true;
  }
  return false;
}

ShaderHandle GraphicsRenderer::CreateShader(const MemoryRegion* mem, ShaderInfo::Header* out_header) {
  ShaderHandle handle = _shader_handles.Alloc();
  if (!handle) {
    c3_log("Failed to alloc shader handle.\n");
    return handle;
  }
  ShaderRef& shader = _shader_ref[handle.idx];
  shader.ref_count = 1;
  shader.num_constants = 0;
  shader.hash = 0;
  shader.owned = false;

  BlobReader blob(mem->data, mem->size);
  ShaderInfo::Header header;
  blob.Read(header);
  c3_assert_return_x(header.magic == C3_CHUNK_MAGIC_VSH || header.magic == C3_CHUNK_MAGIC_FSH, ShaderHandle());
  shader.constants = (ConstantHandle*)C3_ALLOC(g_allocator, header.num_constants * sizeof(ConstantHandle));
  for (u8 i = 0; i < header.num_constants; ++i) {
    auto& c = header.constants[i];
    if (PredefinedConstant::NameToType(c.name) == PREDEFINED_CONSTANT_COUNT) {
      shader.constants[shader.num_constants] = CreateConstant(c.name, (ConstantType)c.constant_type, c.num);
      ++shader.num_constants;
    }
  }
  _gi->CreateShader(handle, mem);
  if (out_header) memcpy(out_header, &header, sizeof(header));
  return handle;
}

void GraphicsRenderer::DestroyShader(ShaderHandle handle) {
  if (!handle) return;
  _gi->DestroyShader(handle);
  ShaderDecRef(handle);
}

ProgramHandle GraphicsRenderer::CreateProgram(ShaderHandle vsh, ShaderHandle fsh, bool destroy_shaders) {
  if (!vsh || !fsh) {
    c3_log("Vertex/fragment shader is invalid (vsh %d, fsh %d).\n", vsh.idx, fsh);
    return ProgramHandle();
  }

  ProgramMap::const_iterator it = _program_map.find(u32(fsh.idx << 16) | vsh.idx);
  if (it != _program_map.end()) {
    ProgramHandle handle = it->second;
    ProgramRef& pr = _program_ref[handle.idx];
    ++pr.ref_count;
    return handle;
  }

  const ShaderRef& vsr = _shader_ref[vsh.idx];
  const ShaderRef& fsr = _shader_ref[fsh.idx];
  if (vsr.hash != fsr.hash) {
    c3_log("Vertex shader output doesn't match fragment shader input.\n");
    return ProgramHandle();
  }

  ProgramHandle handle = _program_handles.Alloc();

  if (!handle) c3_log("Failed to allocate program handle.\n");
  else {
    ShaderIncRef(vsh);
    ShaderIncRef(fsh);
    ProgramRef& pr = _program_ref[handle.idx];
    pr.vsh = vsh;
    pr.fsh = fsh;
    pr.ref_count = 1;

    _program_map.insert(make_pair(u32(fsh.idx << 16) | vsh.idx, handle));

    _gi->CreateProgram(handle, vsh, fsh);
  }

  if (destroy_shaders) {
    ShaderOwn(vsh);
    ShaderOwn(fsh);
  }

  return handle;
}

void GraphicsRenderer::DestroyProgram(ProgramHandle handle) {
  ProgramRef& pr = _program_ref[handle.idx];
  i16 refs = --pr.ref_count;
  if (refs == 0) {
    _gi->DestroyProgram(handle);
    _program_handles.Free(handle);

    ShaderDecRef(pr.vsh);
    u32 hash = pr.vsh.idx;

    if (pr.fsh) {
      ShaderDecRef(pr.fsh);
      hash |= u32(pr.fsh) << 16;
    }

    _program_map.erase(hash);
  }
}

TextureHandle GraphicsRenderer::CreateTexture(const MemoryRegion* mem, u32 flags, u8 skip,
                                              TextureInfo* info_out, BackbufferRatio ratio) {
  TextureInfo ti;
  if (!info_out) info_out = &ti;

  ImageContainer image_container;
  if (image_parse(image_container, mem->data, mem->size)) {
    calc_texture_size(*info_out,
                      image_container.width, image_container.height, image_container.depth,
                      image_container.cube_map, image_container.num_mips,
                      (TextureFormat)image_container.format);
  } else {
    info_out->format = INVALID_TEXTURE_FORMAT;
    info_out->storage_size = 0;
    info_out->width = 0;
    info_out->height = 0;
    info_out->depth = 0;
    info_out->num_mips = 0;
    info_out->bits_per_pixel = 0;
    info_out->cube_map = false;
  }

  TextureHandle handle = _texture_handles.Alloc();
  if (!handle) c3_log("Failed to allocate texture handle.\n");
  else {
    TextureRef& ref = _texture_ref[handle.idx];
    ref.ref_count = 1;
    ref.bb_ratio = u8(ratio);
    ref.format = u8(info_out->format);
    ref.owned = false;

    _gi->CreateTexture(handle, mem, flags, skip);
  }

  return handle;
}

TextureHandle GraphicsRenderer::CreateTexture2D(u16 width, u16 height, u8 mipmap_count,
                                                TextureFormat format, u32 flags,
                                                const MemoryRegion* mem, TextureInfo* info_out) {
  return CreateTexture2D(BACKBUFFER_RATIO_COUNT, width, height, mipmap_count, format, flags, mem, info_out);
}

TextureHandle GraphicsRenderer::CreateTexture2D(BackbufferRatio ratio, u8 mipmap_count,
                                                TextureFormat format, u32 flags,
                                                TextureInfo* info_out) {
  return CreateTexture2D(ratio, 0, 0, mipmap_count, format, flags, nullptr, info_out);
}

TextureHandle GraphicsRenderer::CreateTexture2D(BackbufferRatio ratio, u16 width, u16 height,
                                                u8 num_mips, TextureFormat format, u32 flags,
                                                const MemoryRegion* data_mem,
                                                TextureInfo* info_out) {
#if C3_DEBUG_CONTEXT
  if (data_mem) {
    TextureInfo ti;
    calc_texture_size(ti, width, height, 1, false, num_mips, format);
    if (ti.storage_size != data_mem->size) {
      c3_log("CreateTexture2D: Texture storage size doesn't match passed memory size (storage size: %d, memory size: %d)\n",
             ti.storage_size, data_mem->size);
      return TextureHandle();
    }
  }
#endif

  BlobWriter stream;
  stream.Reserve(4 + sizeof(TextureCreate));
  u32 magic = C3_CHUNK_MAGIC_TEX;
  stream.Write(magic);
  if (ratio != BACKBUFFER_RATIO_COUNT) {
    width = _resolution.width;
    height = _resolution.height;
    get_texture_size_from_ratio(ratio, width, height);
  }

  TextureCreate tc;
  tc.flags = flags;
  tc.width = width;
  tc.height = height;
  tc.sides = 0;
  tc.depth = 0;
  tc.num_mips = num_mips;
  tc.format = u8(format);
  tc.cube_map = false;
  tc.mem = data_mem;
  stream.Write(tc);

  return CreateTexture(mem_copy(stream.GetData(), stream.GetCapacity()), flags, 0, info_out, ratio);
}

void GraphicsRenderer::DestroyTexture(TextureHandle handle) {
  if (!handle) return;
  TextureDecRef(handle);
}

ConstantHandle GraphicsRenderer::CreateConstant(stringid name, ConstantType type, u16 num) {
  if (PredefinedConstant::NameToType(name) != PREDEFINED_CONSTANT_COUNT) {
    c3_log("%s is predefined uniform name.\n", name);
    return ConstantHandle();
  }

  ConstantMap::iterator it = _constant_map.find(name);
  if (it != _constant_map.end()) {
    ConstantHandle handle = it->second;
    ConstantRef& constant = _constant_ref[handle.idx];

    u32 old_size = CONSTANT_TYPE_SIZE[constant.type];
    u32 new_size = CONSTANT_TYPE_SIZE[type];

    if (old_size < new_size || constant.num < num) {
      constant.type = old_size < new_size ? type : constant.type;
      constant.num = max(constant.num, num);

      _gi->CreateConstant(handle, constant.type, constant.num, name);
    }

    ++constant.ref_count;
    return handle;
  }

  ConstantHandle handle = _constant_handles.Alloc();

  if (!handle) c3_log("Failed to allocate constant handle.\n");
  else {
    //c3_log("Creating uniform (handle %3d) %s\n", handle.idx, name);

    ConstantRef& constant = _constant_ref[handle.idx];
    constant.ref_count = 1;
    constant.type = type;
    constant.num = num;

    _constant_map.insert(make_pair(name, handle));

    _gi->CreateConstant(handle, type, num, name);
  }

  return handle;
}

void GraphicsRenderer::DestroyConstant(ConstantHandle handle) {
  ConstantRef& constant = _constant_ref[handle.idx];
  i16 refs = --constant.ref_count;

  if (refs == 0) {
    for (ConstantMap::iterator it = _constant_map.begin(), it_end = _constant_map.end(); it != it_end; ++it) {
      if (it->second == handle) {
        _constant_map.erase(it);
        break;
      }
    }

    _gi->DestroyConstant(handle);
    _constant_handles.Free(handle);
  }
}

FrameBufferHandle GraphicsRenderer::CreateFrameBuffer(u8 num, TextureHandle* handles,
                                                      bool destroy_textures) {
  FrameBufferHandle handle = _frame_buffer_handles.Alloc();
  if (!handle) c3_log("Failed to allocate frame buffer handle.\n");
  else {
    FrameBufferRef& ref = _frame_buffer_ref[handle.idx];

    ref.window = false;
    memset(ref.th, 0xff, sizeof(ref.th));
    BackbufferRatio bb_ratio = (BackbufferRatio)_texture_ref[handles[0].idx].bb_ratio;
    for (u32 i = 0; i < num; ++i) {
      TextureHandle th = handles[i];
      if (bb_ratio != _texture_ref[th.idx].bb_ratio) c3_log("Mismatch in texture back-buffer ratio.\n");
      ref.th[i] = th;
      TextureIncRef(th);
    }

    _gi->CreateFrameBuffer(handle, num, handles);
  }

  if (destroy_textures) {
    for (u32 i = 0; i < num; ++i) TextureOwn(handles[i]);
  }

  return handle;
}

void GraphicsRenderer::DestroyFrameBuffer(FrameBufferHandle handle) {
  if (!handle) return;
  _gi->DestroyFrameBuffer(handle);
  _frame_buffer_handles.Free(handle);
  FrameBufferRef& ref = _frame_buffer_ref[handle.idx];
  if (!ref.window) {
    for (u32 ii = 0; ii < ARRAY_SIZE(ref.th); ++ii) {
      auto th = ref.th[ii];
      if (th) TextureDecRef(th);
    }
  }
}

void GraphicsRenderer::ResizeTexture(TextureHandle handle, u16 width, u16 height) {
  const TextureRef& texture_ref = _texture_ref[handle.idx];
  get_texture_size_from_ratio((BackbufferRatio)texture_ref.bb_ratio, width, height);

  _gi->ResizeTexture(handle, width, height);
}

void GraphicsRenderer::UpdateTexture2D(TextureHandle handle, u8 mip, u16 x, u16 y, u16 width, u16 height, const MemoryRegion* mem, u16 pitch) {
  c3_assert_return(mem && "mem can't be NULL");
  if (width == 0 || height == 0) mem_free(mem);
  else {
    TextureRect rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    _gi->UpdateTexture(handle, 8, mip, rect, 0, 1, pitch, mem);
  }
}

u16 GraphicsRenderer::SetTransform(const float4x4* mtx, u16 num) {
  return _frame->SetTransform(mtx, num);
}

void GraphicsRenderer::SetTransform(u16 cache, u16 num) {
  _frame->SetTransform(cache, num);
}

const InstanceDataBuffer* GraphicsRenderer::AllocInstanceDataBuffer(u32 num, u16 stride) {
  TransientVertexBuffer& dvb = *_frame->transient_vb;

  u32 out_num = num;
  u32 offset = _frame->AllocTransientVertexBuffer(out_num, stride);
  if (out_num != num) c3_log("[C3] Failed to allocate instance data buffer: num=%d stride=%hd.\n", num, stride);
  InstanceDataBuffer* idb = (InstanceDataBuffer*)C3_ALLOC(g_allocator, sizeof(InstanceDataBuffer));
  idb->data = &dvb.data[offset];
  idb->size = num * stride;
  idb->offset = offset;
  idb->num = num;
  idb->stride = stride;
  idb->handle = dvb.handle;
  return idb;
}

bool GraphicsRenderer::CheckAvailInstanceDataBuffer(u32 num, u16 stride) {
  return _frame->CheckAvailTransientVertexBuffer(num, stride);
}

u16 GraphicsRenderer::AllocTransform(float4x4*& mtx_out, u16& num_in_out) {
  return _frame->AllocTransform(mtx_out, num_in_out);
}

void GraphicsRenderer::SetVertexBuffer(VertexBufferHandle handle, u32 start, u32 num) {
  _frame->SetVertexBuffer(handle, start, num);
}

void GraphicsRenderer::SetVertexBuffer(const TransientVertexBuffer* tvb, u32 start_vertex, u32 num_vertices) {
  _frame->SetVertexBuffer(tvb, start_vertex, num_vertices);
}

void GraphicsRenderer::SetIndexBuffer(IndexBufferHandle handle, u32 start, u32 num) {
  _frame->SetIndexBuffer(handle, start, num);
}

void GraphicsRenderer::SetIndexBuffer(const TransientIndexBuffer* tib, u32 first_index, u32 num_indices) {
  _frame->SetIndexBuffer(tib, first_index, num_indices);
}

void GraphicsRenderer::SetScissor(i16 x, i16 y, i16 width, i16 height) {
  _frame->SetScissor(x, y, width, height);
}

void GraphicsRenderer::SetConstant(ConstantHandle handle, const void* value, u16 num) {
  if (!handle) return;
  ConstantRef& constant = _constant_ref[handle.idx];
  _frame->SetConstant(handle, constant.type, value, min(num, constant.num));
}

void GraphicsRenderer::SetTexture(u8 unit, FrameBufferHandle handle, int idx, u32 flags) {
  TextureHandle texture_handle;
  if (handle) {
    const FrameBufferRef& ref = _frame_buffer_ref[handle.idx];
    c3_assert_return(!ref.window && "Can't sample window frame buffer.");
    texture_handle = ref.th[idx];
    c3_assert_return(texture_handle && "Frame buffer texture is invalid.");
  }
  _frame->SetTexture(unit, texture_handle, flags);
}

void GraphicsRenderer::SetTexture(u8 unit, TextureHandle handle, u32 flags) {
  _frame->SetTexture(unit, handle, flags);
}

void GraphicsRenderer::SetViewRect(u8 view, u16 x, u16 y, u16 width, u16 height) {
  _rect[view].left = x;
  _rect[view].bottom = y;
  _rect[view].right = x + width;
  _rect[view].top = y + height;
}

void GraphicsRenderer::SetViewScissor(u8 view, i16 x, i16 y, i16 width, i16 height) {
  _scissor[view].left = x;
  _scissor[view].bottom = y;
  _scissor[view].right = x + width;
  _scissor[view].top = y + height;
}

void GraphicsRenderer::SetViewSeq(u8 view, bool enable) {
  _seq_enabled[view] = enable;
}

void GraphicsRenderer::SetViewClear(u8 view, u16 flags, u32 rgba, float depth, u8 stencil) {
  auto& c = _view_clear[view];
  c.index[0] = u8(rgba >> 24);
  c.index[1] = u8(rgba >> 16);
  c.index[2] = u8(rgba >> 8);
  c.index[3] = u8(rgba);
  c.flags = flags;
  c.depth = depth;
  c.stencil = stencil;
}

void GraphicsRenderer::SetViewClear(u8 view, u16 flags, float depth, u8 stencil,
                                    u8 attach0, u8 attach1, u8 attach2, u8 attach3,
                                    u8 attach4, u8 attach5, u8 attach6, u8 attach7) {
  auto& c = _view_clear[view];
  c.index[0] = attach0;
  c.index[1] = attach1;
  c.index[2] = attach2;
  c.index[3] = attach3;
  c.index[4] = attach4;
  c.index[5] = attach5;
  c.index[6] = attach6;
  c.index[7] = attach7;
  c.flags = (flags & ~C3_CLEAR_COLOR) |
    ((attach0 & attach1 & attach2 & attach3 & attach4 & attach5 & attach6 & attach7) != UINT8_MAX ? C3_CLEAR_COLOR | C3_CLEAR_COLOR_USE_PALETTE : 0);
  c.depth = depth;
  c.stencil = stencil;
}

void GraphicsRenderer::SetViewFrameBuffer(u8 view, FrameBufferHandle fbh) {
  _fb[view] = fbh;
}

void GraphicsRenderer::SetViewTransform(u8 view, const float* view_matrix, const float* proj_left, u8 flags, const float* proj_right) {
  _view_flags[view] = flags;
  if (view_matrix) memcpy(&_view[view], view_matrix, sizeof(float) * 16);
  else _view[view].SetIdentity();
  if (proj_left) memcpy(&_proj[0][view], proj_left, sizeof(float) * 16);
  else _proj[0][view].SetIdentity();
  if (proj_right) memcpy(&_proj[1][view], proj_right, sizeof(float) * 16);
  else _proj[1][view].SetIdentity();
}

void GraphicsRenderer::SetViewRemap(u8 start_view, u8 num, u8* remap) {
  u8 end_view = min<u8>(start_view + num, C3_MAX_VIEWS);
  if (!remap) {
    for (u8 i = start_view; i < end_view; ++i) _view_remap[i] = i;
  } else {
    memcpy(_view_remap + start_view, remap, end_view - start_view);
  }
}

void GraphicsRenderer::SetViewName(u8 view, const char* name) {
  _gi->UpdateViewName(view, name);
}

void GraphicsRenderer::SetPaletteColor(u8 index, u32 rgba) {
  const u8 r = u8(rgba >> 24);
  const u8 g = u8(rgba >> 16);
  const u8 b = u8(rgba >> 8);
  const u8 a = u8(rgba >> 0);

  float rgba_arr[4] = {
    r * 1.0f / 255.0f,
    g * 1.0f / 255.0f,
    b * 1.0f / 255.0f,
    a * 1.0f / 255.0f,
  };
  SetPaletteColor(index, rgba_arr);
}

void GraphicsRenderer::SetPaletteColor(u8 index, float r, float g, float b, float a) {
  float rgba[4] = {r, g, b, a};
  SetPaletteColor(index, rgba);
}

void GraphicsRenderer::SetPaletteColor(u8 index, const float rgba[4]) {
  memcpy(&_color_palette[index][0], rgba, 16);
  _color_palette_dirty = 2;
}

void GraphicsRenderer::SetPaletteColor(u8 index, const Color& color) {
  SetPaletteColor(index, (const float*)&color);
}

void GraphicsRenderer::SetState(u64 state, u32 rgba) {
  _frame->SetState(state, rgba);
}

void GraphicsRenderer::SetMarker(const char* marker) {
  _frame->SetMarker(marker);
}

void GraphicsRenderer::Submit(u8 view, ProgramHandle program, i32 tag) {
  _frame->Submit(view, program, tag);
}

void GraphicsRenderer::Discard() {
  _frame->Discard();
}

void GraphicsRenderer::Frame() {
  FrameNoRenderWait();
}

i32 GraphicsRenderer::RenderOneFrame() {
  if (_gi) _gi->Flip();
  //auto start_time = get_timestamp();
  _gi->Submit(_frame, _clear_quad);
  /*
  auto elapsed_msecs = (get_timestamp() - start_time) * 1000.0;
  if (elapsed_msecs >= 17.0) {
  c3_log("[WARN] Time budget exceeds: %.3lf ms.\n", elapsed_msecs);
  }
  */
  return 1;
}

void GraphicsRenderer::Swap() {
  _frame->resolution = _resolution;
  memcpy(_frame->view_remap, _view_remap, sizeof(_view_remap));
  memcpy(_frame->fb, _fb, sizeof(_fb));
  memcpy(_frame->view_clear, _view_clear, sizeof(_view_clear));
  memcpy(_frame->rect, _rect, sizeof(_rect));
  memcpy(_frame->scissor, _scissor, sizeof(_scissor));
  memcpy(_frame->_view, _view, sizeof(_view));
  memcpy(_frame->proj, _proj, sizeof(_proj));
  memcpy(_frame->view_flags, _view_flags, sizeof(_view_flags));
  if (_color_palette_dirty > 0) {
    --_color_palette_dirty;
    memcpy(_frame->color_palette, _color_palette, sizeof(_color_palette));
  }
  _frame->Finish();
  
  ++_frame_counter;
  RenderOneFrame();

  _frame->Start();
  memset(_fb, 0xff, sizeof(_fb));
  memset(_seq, 0, sizeof(_seq));
  memset(_seq_enabled, 0, sizeof(_seq_enabled));
  _num_views = 0;
  _current_view = 0;
  memset(_view_clear, 0, sizeof(_view_clear));
  for (u8 i = 0; i < C3_MAX_VIEWS; ++i) _view_remap[i] = i;
  FreeAllHandles(_frame);
  _frame->ResetFreeHandles();
}

void GraphicsRenderer::FrameNoRenderWait() {
  Swap();
}

void GraphicsRenderer::TextureIncRef(TextureHandle handle) {
  ++_texture_ref[handle.idx].ref_count;
}

void GraphicsRenderer::TextureDecRef(TextureHandle handle) {
  TextureRef& ref = _texture_ref[handle.idx];
  i16 refs = --ref.ref_count;
  if (refs == 0) _texture_handles.Free(handle);
}

void GraphicsRenderer::TextureOwn(TextureHandle handle) {
  TextureRef& ref = _texture_ref[handle.idx];
  if (!ref.owned) {
    ref.owned = true;
    TextureDecRef(handle);
  }
}

void GraphicsRenderer::ShaderIncRef(ShaderHandle handle) {
  ++_shader_ref[handle.idx].ref_count;
}

void GraphicsRenderer::ShaderDecRef(ShaderHandle handle) {
  ShaderRef& ref = _shader_ref[handle.idx];
  i16 refs = --ref.ref_count;
  if (refs == 0) {
    _gi->DestroyShader(handle);
    _shader_handles.Free(handle);
  }
}

void GraphicsRenderer::ShaderOwn(ShaderHandle handle) {
  ShaderRef& ref = _shader_ref[handle.idx];
  if (!ref.owned) {
    ref.owned = true;
    ShaderDecRef(handle);
  }
}

VertexDeclHandle GraphicsRenderer::FindVertexDecl(const VertexDecl& decl) {
  VertexDeclHandle decl_handle = _decl_ref.Find(decl.hash);

  if (!decl_handle) {
    decl_handle = _vertex_decl_handles.Alloc();
    _gi->CreateVertexDecl(decl_handle, decl);
  }

  return decl_handle;
}

void GraphicsRenderer::FreeAllHandles(RenderFrame* frame) {
}

TransientIndexBuffer* GraphicsRenderer::CreateTransientIndexBuffer(u32 size) {
  TransientIndexBuffer* tib = nullptr;

  IndexBufferHandle handle = _index_buffer_handles.Alloc();
  if (!handle) c3_log("Failed to allocate transient index buffer handle.\n");
  else {
    _gi->CreateDynamicIndexBuffer(handle, size, C3_BUFFER_NONE);

    tib = (TransientIndexBuffer*)C3_ALLOC(g_allocator, sizeof(TransientIndexBuffer) + size);
    tib->data = (u8*)&tib[1];
    tib->size = size;
    tib->handle = handle;
  }

  return tib;
}

void GraphicsRenderer::DestroyTransientIndexBuffer(TransientIndexBuffer* tib) {
  _gi->DestroyIndexBuffer(tib->handle);
  _index_buffer_handles.Free(tib->handle);
  C3_FREE(g_allocator, tib);
}

TransientVertexBuffer* GraphicsRenderer::CreateTransientVertexBuffer(u32 size, const VertexDecl* decl) {
  TransientVertexBuffer* tvb = NULL;

  VertexBufferHandle handle = _vertex_buffer_handles.Alloc();

  if (!handle) c3_log("Failed to allocate transient vertex buffer handle.\n");
  else {
    u16 stride = 0;
    VertexDeclHandle decl_handle;

    if (decl) {
      decl_handle = FindVertexDecl(*decl);
      _decl_ref.Add(decl_handle, decl->hash);

      stride = decl->stride;
    }

    _gi->CreateDynamicVertexBuffer(handle, size, C3_BUFFER_NONE);

    tvb = (TransientVertexBuffer*)C3_ALLOC(g_allocator, sizeof(TransientVertexBuffer) + size);
    tvb->data = (u8*)&tvb[1];
    tvb->size = size;
    tvb->start_vertex = 0;
    tvb->stride = stride;
    tvb->handle = handle;
    tvb->decl = decl_handle;
  }

  return tvb;
}

void GraphicsRenderer::DestroyTransientVertexBuffer(TransientVertexBuffer* tvb) {
  _gi->DestroyVertexBuffer(tvb->handle);
  _vertex_buffer_handles.Free(tvb->handle);
  C3_FREE(g_allocator, tvb);
}

void calc_texture_size(TextureInfo& info_out, u16 width_, u16 height_, u16 depth_, bool cube_map, u8 num_mips, TextureFormat format) {
  const ImageBlockInfo& ibi = image_block_info(format);
  const u8 bpp = ibi.bits_per_pixel;
  const u16 block_width = ibi.block_width;
  const u16 block_height = ibi.block_height;
  const u16 min_block_x = ibi.min_block_x;
  const u16 min_block_y = ibi.min_block_y;

  width_ = max<u16>(block_width  * min_block_x, ((width_ + block_width - 1) / block_width) * block_width);
  height_ = max<u16>(block_height * min_block_y, ((height_ + block_height - 1) / block_height) * block_height);
  depth_ = max<u16>(1, depth_);
  num_mips = max<u8>(1, num_mips);

  u32 width = width_;
  u32 height = height_;
  u32 depth = max<u32>(1, depth_);
  u32 sides = cube_map ? 6 : 1;
  u32 size = 0;

  for (u8 lod = 0; lod < num_mips; ++lod) {
    u32 lod_width = max<u32>(block_width  * min_block_x, (width + block_width - 1) / block_width * block_width);
    u32 lod_height = max<u32>(block_height * min_block_y, (height + block_height - 1) / block_height * block_height);
    size += lod_width * lod_height * depth * bpp / 8 * sides;

    width = max<u32>(1, width / 2);
    height = max<u32>(1, height / 2);
    depth = max<u32>(1, depth / 2);
  }

  info_out.format = format;
  info_out.width = width_;
  info_out.height = height_;
  info_out.depth = depth_;
  info_out.num_mips = num_mips;
  info_out.cube_map = cube_map;
  info_out.storage_size = size;
  info_out.bits_per_pixel = bpp;
}

void get_texture_size_from_ratio(BackbufferRatio ratio, u16& width_out, u16& height_out) {
  switch (ratio) {
  case BACKBUFFER_RATIO_HALF:
    width_out /= 2;
    height_out /= 2;
    break;
  case BACKBUFFER_RATIO_QUARTER:
    width_out /= 4;
    height_out /= 4;
    break;
  case BACKBUFFER_RATIO_EIGHTH:
    width_out /= 8;
    height_out /= 8;
    break;
  case BACKBUFFER_RATIO_SIXTEENTH:
    width_out /= 16;
    height_out /= 16;
    break;
  case BACKBUFFER_RATIO_DOUBLE:
    width_out *= 2;
    height_out *= 2;
    break;
  default:
    break;
  }

  width_out = max<u16>(width_out, 1);
  height_out = max<u16>(height_out, 1);
}
