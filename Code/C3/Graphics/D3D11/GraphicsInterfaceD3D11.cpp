//
//  GraphicsInterfaceDx.cpp
//  ShadowPlay
//
//  Created by hoi wang on 10-30-15
//
//
#include "C3PCH.h"
#include "GraphicsInterfaceD3D11.h"
#include "dx_graphics.h"
#include "Graphics/ConstantBuffer.h"
#include "Graphics/GraphicsRenderer.h"
#include "Graphics/ViewState.h"
#include "Graphics/Image/ImageUtils.h"
#include "Data/Blob.h"
#include "Debug/C3Debug.h"

GraphicsInterfaceD3D11 *g_interface = nullptr;

union Zero {
  Zero() {
    memset(this, 0, sizeof(Zero));
  }

  ID3D11Buffer*              m_buffer[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11UnorderedAccessView* m_uav[D3D11_PS_CS_UAV_REGISTER_COUNT];
  ID3D11ShaderResourceView*  m_srv[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
  ID3D11SamplerState*        m_sampler[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
  ID3D11RenderTargetView*    m_rtv[C3_MAX_FRAME_BUFFERS];
  u32                   m_zero[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
  float                      m_zerof[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
};

static const Zero s_zero;

static const u32 s_checkMsaa[] = {0, 2, 4, 8, 16};

static DXGI_SAMPLE_DESC s_msaa[] = {
  {1, 0},
  {2, 0},
  {4, 0},
  {8, 0},
  {16, 0},
};

static const D3D11_BLEND s_blendFactor[][2] = {
  {(D3D11_BLEND)0, (D3D11_BLEND)0}, // ignored
  {D3D11_BLEND_ZERO, D3D11_BLEND_ZERO}, // ZERO
  {D3D11_BLEND_ONE, D3D11_BLEND_ONE},	// ONE
  {D3D11_BLEND_SRC_COLOR, D3D11_BLEND_SRC_ALPHA},	// SRC_COLOR
  {D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_INV_SRC_ALPHA},	// INV_SRC_COLOR
  {D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_SRC_ALPHA},	// SRC_ALPHA
  {D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA},	// INV_SRC_ALPHA
  {D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_DEST_ALPHA},	// DST_ALPHA
  {D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA},	// INV_DST_ALPHA
  {D3D11_BLEND_DEST_COLOR, D3D11_BLEND_DEST_ALPHA},	// DST_COLOR
  {D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_INV_DEST_ALPHA},	// INV_DST_COLOR
  {D3D11_BLEND_SRC_ALPHA_SAT, D3D11_BLEND_ONE},	// SRC_ALPHA_SAT
  {D3D11_BLEND_BLEND_FACTOR, D3D11_BLEND_BLEND_FACTOR},	// FACTOR
  {D3D11_BLEND_INV_BLEND_FACTOR, D3D11_BLEND_INV_BLEND_FACTOR},	// INV_FACTOR
};

static const D3D11_BLEND_OP s_blendEquation[] = {
  D3D11_BLEND_OP_ADD,
  D3D11_BLEND_OP_SUBTRACT,
  D3D11_BLEND_OP_REV_SUBTRACT,
  D3D11_BLEND_OP_MIN,
  D3D11_BLEND_OP_MAX,
};

static const D3D11_COMPARISON_FUNC s_cmpFunc[] = {
  D3D11_COMPARISON_FUNC(0), // ignored
  D3D11_COMPARISON_LESS,
  D3D11_COMPARISON_LESS_EQUAL,
  D3D11_COMPARISON_EQUAL,
  D3D11_COMPARISON_GREATER_EQUAL,
  D3D11_COMPARISON_GREATER,
  D3D11_COMPARISON_NOT_EQUAL,
  D3D11_COMPARISON_NEVER,
  D3D11_COMPARISON_ALWAYS,
};

static const D3D11_STENCIL_OP s_stencilOp[] = {
  D3D11_STENCIL_OP_ZERO,
  D3D11_STENCIL_OP_KEEP,
  D3D11_STENCIL_OP_REPLACE,
  D3D11_STENCIL_OP_INCR,
  D3D11_STENCIL_OP_INCR_SAT,
  D3D11_STENCIL_OP_DECR,
  D3D11_STENCIL_OP_DECR_SAT,
  D3D11_STENCIL_OP_INVERT,
};

static const D3D11_CULL_MODE s_cullMode[] = {
  D3D11_CULL_NONE,
  D3D11_CULL_FRONT,
  D3D11_CULL_BACK,
};

static const D3D11_TEXTURE_ADDRESS_MODE s_textureAddress[] = {
  D3D11_TEXTURE_ADDRESS_WRAP,
  D3D11_TEXTURE_ADDRESS_MIRROR,
  D3D11_TEXTURE_ADDRESS_CLAMP,
  D3D11_TEXTURE_ADDRESS_BORDER,
};

/*
 * D3D11_FILTER_MIN_MAG_MIP_POINT               = 0x00,
 * D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR        = 0x01,
 * D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x04,
 * D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR        = 0x05,
 * D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT        = 0x10,
 * D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11,
 * D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT        = 0x14,
 * D3D11_FILTER_MIN_MAG_MIP_LINEAR              = 0x15,
 * D3D11_FILTER_ANISOTROPIC                     = 0x55,
 *
 * D3D11_COMPARISON_FILTERING_BIT               = 0x80,
 * D3D11_ANISOTROPIC_FILTERING_BIT              = 0x40,
 *
 * According to D3D11_FILTER enum bits for mip, mag and mip are:
 * 0x10 // MIN_LINEAR
 * 0x04 // MAG_LINEAR
 * 0x01 // MIP_LINEAR
 */

static const u8 s_textureFilter[3][3] = {
  {
    0x10, // min linear
    0x00, // min point
    0x55, // anisotropic
  },
  {
    0x04, // mag linear
    0x00, // mag point
    0x55, // anisotropic
  },
  {
    0x01, // mip linear
    0x00, // mip point
    0x55, // anisotropic
  },
};

struct TextureFormatInfo {
  DXGI_FORMAT m_fmt;
  DXGI_FORMAT m_fmtSrv;
  DXGI_FORMAT m_fmtDsv;
  DXGI_FORMAT m_fmtSrgb;
};

static const TextureFormatInfo s_textureFormat[] = {
  {DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RED_8_TEXTURE_FORMAT
  {DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RG_8_TEXTURE_FORMAT
  {DXGI_FORMAT_B8G8R8X8_TYPELESS, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB}, // RGB_8_TEXTURE_FORMAT
  {DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB}, // RGBA_8_TEXTURE_FORMAT
  {DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RED_32_FLOAT_TEXTURE_FORMAT
  {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RGB_16_FLOAT_TEXTURE_FORMAT
  {DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RGBA_16_FLOAT_TEXTURE_FORMAT
  {DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RGB_32_FLOAT_TEXTURE_FORMAT
  {DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN}, // RGBA_32_FLOAT_TEXTURE_FORMAT
  {DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_UNKNOWN}, // DEPTH_16_TEXTURE_FORMAT
  {DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_UNKNOWN}, // DEPTH_24_TEXTURE_FORMAT
  {DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_UNKNOWN}, // DEPTH_32_TEXTURE_FORMAT
  {DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_BC1_UNORM_SRGB}, // DXT1_RGB_TEXTURE_FORMAT
  {DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_BC1_UNORM_SRGB}, // DXT1_ARGB_TEXTURE_FORMAT
  {DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_BC2_UNORM_SRGB}, // DXT3_ARGB_TEXTURE_FORMAT
  {DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_BC3_UNORM_SRGB}, // DXT5_ARGB_TEXTURE_FORMAT
};

static const D3D11_INPUT_ELEMENT_DESC s_attrib[] =
{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"COLOR", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"INDEX", 0, DXGI_FORMAT_R8G8B8A8_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"WEIGHT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"DATA", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"DATA", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"DATA", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"DATA", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
  {"DATA", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

static const DXGI_FORMAT s_attribType[][4][2] =
{
  { // INT8_TYPE
    {DXGI_FORMAT_R8_SINT, DXGI_FORMAT_R8_SNORM},
    {DXGI_FORMAT_R8G8_SINT, DXGI_FORMAT_R8G8_SNORM},
    {DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SNORM},
    {DXGI_FORMAT_R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SNORM},
  },
  { // UINT8_TYPE
    {DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R8_UNORM},
    {DXGI_FORMAT_R8G8_UINT, DXGI_FORMAT_R8G8_UNORM},
    {DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UNORM},
    {DXGI_FORMAT_R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UNORM},
  },
  { // INT16_TYPE
    {DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16_SNORM},
    {DXGI_FORMAT_R16G16_SINT, DXGI_FORMAT_R16G16_SNORM},
    {DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SNORM},
    {DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SNORM},
  },
  { // UINT16_TYPE
    {DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R16_UNORM},
    {DXGI_FORMAT_R16G16_UINT, DXGI_FORMAT_R16G16_UNORM},
    {DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UNORM},
    {DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UNORM},
  },
  { // INT32_TYPE
    {DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32_FLOAT},
    {DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32_FLOAT},
    {DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_FLOAT},
    {DXGI_FORMAT_R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_FLOAT},
  },
  { // UINT32_TYPE
    {DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_FLOAT},
    {DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32_FLOAT},
    {DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT},
    {DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_FLOAT},
  },
  { // INT64_TYPE
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
  },
  { // UINT64_TYPE
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
  },
  { // FLOAT_TYPE
    {DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32_FLOAT},
    {DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT},
    {DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT},
    {DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT},
  },
  { // DOUBLE_TYPE
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
    {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN},
  },
};

static bool is_lost(HRESULT hr) {
  return DXGI_ERROR_DEVICE_REMOVED == hr ||
    DXGI_ERROR_DEVICE_HUNG == hr ||
    DXGI_ERROR_DEVICE_RESET == hr ||
    DXGI_ERROR_DRIVER_INTERNAL_ERROR == hr ||
    DXGI_ERROR_NOT_CURRENTLY_AVAILABLE == hr;
}


void BufferD3D11::Create(u32 size, void* data, u16 flags, u16 stride_, bool vertex) {
  _uav = NULL;
  _size = size;
  _flags = flags;

  const bool needSrv = 0 != (flags & C3_BUFFER_COMPUTE_READ);
  const bool drawIndirect = 0 != (flags & C3_BUFFER_DRAW_INDIRECT);
  _dynamic = !data;

  D3D11_BUFFER_DESC desc;
  desc.ByteWidth = size;
  desc.BindFlags = (vertex ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER) |
    (needSrv ? D3D11_BIND_SHADER_RESOURCE : 0);
  desc.MiscFlags = drawIndirect ? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0;
  desc.StructureByteStride = 0;

  DXGI_FORMAT format;
  u32 stride;

  if (drawIndirect) {
    format = DXGI_FORMAT_R32G32B32A32_UINT;
    stride = 16;
  } else {
    if (vertex) {
      format = DXGI_FORMAT_R32G32B32A32_FLOAT;
      stride = 16;
    } else {
      if (!(_flags & C3_BUFFER_INDEX32)) {
        format = DXGI_FORMAT_R16_UINT;
        stride = 2;
      } else {
        format = DXGI_FORMAT_R32_UINT;
        stride = 4;
      }
    }
  }

  ID3D11Device* device = g_interface->_device;

  if (_dynamic) {
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    DX_CHECK(device->CreateBuffer(&desc, NULL, &_ptr));
  } else {
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA srd;
    srd.pSysMem = data;
    srd.SysMemPitch = 0;
    srd.SysMemSlicePitch = 0;

    DX_CHECK(device->CreateBuffer(&desc, &srd, &_ptr));
  }

  if (needSrv) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    srvd.Format = format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = _size / stride;
    DX_CHECK(device->CreateShaderResourceView(_ptr, &srvd, &_srv));
  }
}

void BufferD3D11::Update(u32 offset, u32 size, void* data, bool discard) {
  ID3D11DeviceContext* context = g_interface->_context;
  c3_assert(_dynamic && "Must be dynamic!");

#if 1
  D3D11_BUFFER_DESC desc;
  desc.ByteWidth = size;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;

  D3D11_SUBRESOURCE_DATA dsd;
  dsd.pSysMem = data;
  dsd.SysMemPitch = 0;
  dsd.SysMemSlicePitch = 0;

  ID3D11Buffer* ptr;
  g_interface->_device->CreateBuffer(&desc, &dsd, &ptr);

  D3D11_BOX box;
  box.left = 0;
  box.right = size;
  box.top = 0;
  box.bottom = 1;
  box.front = 0;
  box.back = 1;

  context->CopySubresourceRegion(_ptr, 0, offset, 0, 0, ptr, 0, &box);

  DX_RELEASE(ptr);
#else
  D3D11_MAPPED_SUBRESOURCE mapped;
  D3D11_MAP type = D3D11_MAP_WRITE_NO_OVERWRITE;
  DX_CHECK(context->Map(_ptr, 0, type, 0, &mapped));
  memcpy((u8*)mapped.pData + offset, data, size);
  context->Unmap(_ptr, 0);
#endif
}

void VertexBufferD3D11::Create(u32 size, void* data, VertexDeclHandle decl_handle, u16 flags) {
  _decl = decl_handle;
  u16 stride = decl_handle ? g_interface->_vertex_decls[decl_handle.idx].stride : 0;

  BufferD3D11::Create(size, data, flags, stride, true);
}

void ShaderD3D11::Create(const MemoryRegion* mem) {
  BlobReader blob(mem->data, mem->size);
  ShaderInfo::Header header;

  blob.Read(header);
  c3_assert_return(header.magic == C3_CHUNK_MAGIC_VSH || header.magic == C3_CHUNK_MAGIC_FSH);
  bool fragment = C3_CHUNK_MAGIC_FSH == header.magic;

  _num_predefined = 0;
  _num_uniforms = header.num_constants;

  u8 fragment_bit = fragment ? CONSTANT_FRAGMENTBIT : 0;

  for (u32 ii = 0; ii < header.num_constants; ++ii) {
    auto& c = header.constants[ii];
    const char* kind = "invalid";

    PredefinedConstantType predefined = PredefinedConstant::NameToType(c.name);
    if (PREDEFINED_CONSTANT_COUNT != predefined) {
      kind = "predefined";
      _predefined[_num_predefined].loc = c.loc;
      _predefined[_num_predefined].count = c.num;
      _predefined[_num_predefined].type = u8(predefined | fragment_bit);
      _num_predefined++;
    } else if (!(c.constant_type & CONSTANT_SAMPLERBIT)) {
      const ConstantInfo* info = g_interface->_uniform_reg.Find(c.name);
      if (!info) c3_log("[C3] User defined uniform 'Hash: %08x' is not found, it won't be set.\n", c.name);

      if (info) {
        if (!_constant_buffer) _constant_buffer = ConstantBuffer::Create(1024);

        kind = "user";
        _constant_buffer->WriteConstantHandle((ConstantType)(c.constant_type | fragment_bit), c.loc, info->handle, c.num);
      }
    } else {
      kind = "sampler";
    }
  }
  if (_constant_buffer) _constant_buffer->Finish();

  const char* code = (const char*)mem->data + header.code_offset;

  if (C3_CHUNK_MAGIC_FSH == header.magic) {
    DX_CHECK(g_interface->_device->CreatePixelShader(code, header.code_size, NULL, &_pixel_shader));
    if (!_ptr) {
      c3_log("Failed to create fragment shader.\n");
      exit(-1);
    }
  } else if (C3_CHUNK_MAGIC_VSH == header.magic) {
    _hash = hash_buffer(code, header.code_size);
    _code = mem_copy(code, header.code_size);

    DX_CHECK(g_interface->_device->CreateVertexShader(code, header.code_size, NULL, &_vertex_shader));
    if (!_ptr) {
      c3_log("Failed to create vertex shader.\n");
      exit(-1);
    }
  } else {
    DX_CHECK(g_interface->_device->CreateComputeShader(code, header.code_size, NULL, &_compute_shader));
    if (!_ptr) {
      c3_log("Failed to create compute shader.\n");
      exit(-1);
    }
  }

  memset(_attr_mask, 0, sizeof(_attr_mask));

  for (u32 ii = 0; ii < header.num_inputs; ++ii) {
    auto& input = header.inputs[ii];
    int attr = semantic_to_vertex_attr(input.semantic);

    if (VERTEX_ATTR_COUNT + INSTANCE_ATTR_COUNT != attr) _attr_mask[attr] = UINT16_MAX;
  }

  _byte_width = header.cbuffer_size;
  if (_byte_width > 0) {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = (_byte_width + 0xf) & ~0xf;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    DX_CHECK(g_interface->_device->CreateBuffer(&desc, NULL, &_buffer));
  }
}

void TextureD3D11::Create(const MemoryRegion* mem, u32 flags, u8 skip) {
  ImageContainer image_container;

  if (image_parse(image_container, mem->data, mem->size)) {
    u8 num_mips = image_container.num_mips;
    const u8 start_lod = min<u8>(skip, num_mips - 1);
    num_mips -= start_lod;
    const ImageBlockInfo& block_info = image_block_info(TextureFormat(image_container.format));
    const u32 texture_width = max<u32>(block_info.block_width, image_container.width >> start_lod);
    const u32 texture_height = max<u32>(block_info.block_height, image_container.height >> start_lod);

    _flags = flags;
    _width = texture_width;
    _height = texture_height;
    _depth = image_container.depth;
    _requested_format = (u8)image_container.format;
    _texture_format = (u8)image_container.format;

    const TextureFormatInfo& tfi = s_textureFormat[_requested_format];
    const bool convert = DXGI_FORMAT_UNKNOWN == tfi.m_fmt;

    u8 bpp = image_bits_per_pixel(TextureFormat(_texture_format));
    if (convert) {
      _texture_format = (u8)RGBA_8_TEXTURE_FORMAT; // #todo: actually BGRA8
      bpp = 32;
    }

    if (image_container.cube_map) {
      _type = TextureCube;
    } else if (image_container.depth > 1) {
      _type = Texture3D;
    } else {
      _type = Texture2D;
    }

    _num_mips = num_mips;

    u32 numSrd = num_mips*(image_container.cube_map ? 6 : 1);
    D3D11_SUBRESOURCE_DATA* srd = (D3D11_SUBRESOURCE_DATA*)alloca(numSrd*sizeof(D3D11_SUBRESOURCE_DATA));

    u32 kk = 0;

    const bool compressed = is_compressed(TextureFormat(_texture_format));
    // #todo: actually BGRA8
    const bool swizzle = RGBA_8_TEXTURE_FORMAT == _texture_format && 0 != (_flags & C3_TEXTURE_COMPUTE_WRITE);

    for (u8 side = 0, numSides = image_container.cube_map ? 6 : 1; side < numSides; ++side) {
      u32 width = texture_width;
      u32 height = texture_height;
      u32 depth = image_container.depth;

      for (u8 lod = 0, num = num_mips; lod < num; ++lod) {
        width = max<u32>(1, width);
        height = max<u32>(1, height);
        depth = max<u32>(1, depth);

        ImageMip mip;
        if (image_get_raw_data(image_container, side, lod + start_lod, mem->data, mem->size, mip)) {
          srd[kk].pSysMem = mip.data;

          if (convert) {
            u32 srcpitch = mip.width*bpp / 8;
            u8* temp = (u8*)C3_ALLOC(g_allocator, mip.width * mip.height * bpp / 8);
            image_get_bgra8_data(temp, mip.data, mip.width, mip.height, srcpitch, mip.format);

            srd[kk].pSysMem = temp;
            srd[kk].SysMemPitch = srcpitch;
          } else if (compressed) {
            srd[kk].SysMemPitch = max<u32>(1, (width + 3) >> 2) * 4 * 4 * image_bits_per_pixel((TextureFormat)_texture_format) / 8;
            srd[kk].SysMemSlicePitch = max<u32>(1, (height + 3) >> 2) * srd[kk].SysMemPitch;
          } else {
            srd[kk].SysMemPitch = mip.width * mip.bpp / 8;
          }
          c3_assert(srd[kk].SysMemPitch > 0);

          if (swizzle) {
            // imageSwizzleBgra8(width, height, mip.m_width*4, data, temp);
          }

          srd[kk].SysMemSlicePitch = mip.height * srd[kk].SysMemPitch;
          ++kk;
        }

        width >>= 1;
        height >>= 1;
        depth >>= 1;
      }
    }

    const bool buffer_only = 0 != (_flags&(C3_TEXTURE_RT_BUFFER_ONLY | C3_TEXTURE_READ_BACK));
    const bool compute_write = 0 != (_flags&C3_TEXTURE_COMPUTE_WRITE);
    const bool render_target = 0 != (_flags&C3_TEXTURE_RT_MASK);
    const bool srgb = 0 != (_flags&C3_TEXTURE_SRGB) || image_container.srgb;
    const bool blit = 0 != (_flags&C3_TEXTURE_BLIT_DST);
    const bool read_back = 0 != (_flags&C3_TEXTURE_READ_BACK);
    const u32 msaa_quality = max<u32>(((_flags&C3_TEXTURE_RT_MSAA_MASK) >> C3_TEXTURE_RT_MSAA_SHIFT), 1) - 1;
    const DXGI_SAMPLE_DESC& msaa = s_msaa[msaa_quality];

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    memset(&srvd, 0, sizeof(srvd));

    DXGI_FORMAT tex_format = s_textureFormat[_texture_format].m_fmt;
    DXGI_FORMAT srv_format = s_textureFormat[_texture_format].m_fmtSrv;
    DXGI_FORMAT srv_srgb_format = s_textureFormat[_texture_format].m_fmtSrgb;

    switch (_type) {
    case Texture2D:
    case TextureCube: {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = texture_width;
      desc.Height = texture_height;
      desc.MipLevels = num_mips;
      desc.Format = tex_format;
      desc.SampleDesc = msaa;
      desc.Usage = kk == 0 || blit ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
      desc.BindFlags = buffer_only ? 0 : D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags = 0;

      if (is_depth((TextureFormat)_texture_format)) {
        desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        desc.Usage = D3D11_USAGE_DEFAULT;
      } else if (render_target) {
        desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        desc.Usage = D3D11_USAGE_DEFAULT;
      }

      if (compute_write) {
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        desc.Usage = D3D11_USAGE_DEFAULT;
      }

      if (read_back) {
        desc.BindFlags = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      }

      if (image_container.cube_map) {
        desc.ArraySize = 6;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvd.TextureCube.MipLevels = num_mips;
      } else {
        desc.ArraySize = 1;
        desc.MiscFlags = 0;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = num_mips;
      }

      DX_CHECK(g_interface->_device->CreateTexture2D(&desc, kk == 0 ? NULL : srd, &_texture2d));

      if (_type == Texture2D && msaa_quality > 0) {
        desc.SampleDesc = s_msaa[0];
        DX_CHECK(g_interface->_device->CreateTexture2D(&desc, kk == 0 ? NULL : srd, &_resolved_texture2d));
      }
    } break;

    case Texture3D: {
      D3D11_TEXTURE3D_DESC desc;
      desc.Width = texture_width;
      desc.Height = texture_height;
      desc.Depth = image_container.depth;
      desc.MipLevels = image_container.num_mips;
      desc.Format = srv_format;
      desc.Usage = kk == 0 || blit ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;

      if (compute_write) {
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        desc.Usage = D3D11_USAGE_DEFAULT;
      }

      srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
      srvd.Texture3D.MipLevels = num_mips;

      DX_CHECK(g_interface->_device->CreateTexture3D(&desc, kk == 0 ? NULL : srd, &_texture3d));
    } break;
    }

    if (!buffer_only) {
      ID3D11Resource* res = _resolved_texture2d ? _resolved_texture2d : _ptr;
      srvd.Format = srv_format;
      DX_CHECK(g_interface->_device->CreateShaderResourceView(res, &srvd, &_srv));
      if (srv_srgb_format != DXGI_FORMAT_UNKNOWN) {
        srvd.Format = srv_srgb_format;
        DX_CHECK(g_interface->_device->CreateShaderResourceView(res, &srvd, &_srv_srgb));
      } else _srv_srgb = nullptr;
    }
    if (compute_write) DX_CHECK(g_interface->_device->CreateUnorderedAccessView(_ptr, NULL, &_uav));

    if (convert && 0 != kk) {
      kk = 0;
      for (u8 side = 0, numSides = image_container.cube_map ? 6 : 1; side < numSides; ++side) {
        for (u32 lod = 0, num = num_mips; lod < num; ++lod) {
          C3_FREE(g_allocator, const_cast<void*>(srd[kk].pSysMem));
          ++kk;
        }
      }
    }
  }
}

void TextureD3D11::Destroy() {
  //g_interface->m_srvUavLru.Invalidate(GetHandle().idx);
  DX_RELEASE(_srv);
  DX_RELEASE(_uav);
  DX_RELEASE(_ptr);
  DX_RELEASE(_resolved_texture2d);
}

void TextureD3D11::Update(u8 side, u8 mip, const TextureRect& rect, u16 z, u16 depth, u16 pitch, const MemoryRegion* mem) {
  ID3D11DeviceContext* context = g_interface->_context;

  D3D11_BOX box;
  box.left = rect.x;
  box.top = rect.y;
  box.right = box.left + rect.width;
  box.bottom = box.top + rect.height;
  box.front = z;
  box.back = box.front + depth;

  const u32 subres = mip + (side * _num_mips);
  const u32 bpp = image_bits_per_pixel(TextureFormat(_texture_format));
  const u32 rectpitch = rect.width * bpp / 8;
  u32 srcpitch = UINT16_MAX == pitch ? rectpitch : pitch;

  const bool convert = _texture_format != _requested_format;

  u8* data = (u8*)mem->data;
  u8* temp = NULL;

  if (convert) {
    temp = (u8*)C3_ALLOC(g_allocator, rectpitch*rect.height);
    image_get_bgra8_data(temp, data, rect.width, rect.height, srcpitch, _requested_format);
    data = temp;
  } else if (is_compressed((TextureFormat)_texture_format)) {
    auto w = ALIGN_MASK(rect.width, 3);
    auto h = ALIGN_MASK(rect.height, 3);
    box.right = box.left + w;
    box.bottom = box.top + h;
    srcpitch = max(1, (rect.width + 3) >> 2) * 4 * 4 * image_bits_per_pixel((TextureFormat)_texture_format) / 8;
  }

  context->UpdateSubresource(_ptr, subres, &box, data, srcpitch, 0);

  if (temp) C3_FREE(g_allocator, temp);
}

void TextureD3D11::Commit(u8 stage, u32 flags_, const float palette[][4]) {
  TextureStage& ts = g_interface->_texture_stage;
  u32 flags = (C3_SAMPLER_DEFAULT_FLAGS & flags_) ? _flags : flags_;
  u32 index = (flags & C3_TEXTURE_BORDER_COLOR_MASK) >> C3_TEXTURE_BORDER_COLOR_SHIFT;

  ts._srv[stage] = ((flags & C3_TEXTURE_SRGB) && _srv_srgb) ? _srv_srgb : _srv;
  ts._sampler[stage] = g_interface->GetSamplerState(flags, palette[index]);
}

void TextureD3D11::Resolve() {
  if (_resolved_texture2d) {
    g_interface->_context->ResolveSubresource(_resolved_texture2d, 0,
                                              _texture2d, 0,
                                              s_textureFormat[_texture_format].m_fmtSrv);
  }
}

void FrameBufferD3D11::Create(u8 num, const TextureHandle* handles) {
  memset(_rtv, 0, sizeof(_rtv));
  memset(_srv, 0, sizeof(_srv));
  _dsv = NULL;
  _swap_chain = NULL;

  _num_th = num;
  memcpy(_th, handles, num * sizeof(TextureHandle));

  PostReset();
}

void FrameBufferD3D11::Create(u16 dense_idx, void* nwh, u32 width, u32 height, TextureFormat depth_format) {
  memset(_rtv, 0, sizeof(_rtv));
  memset(_srv, 0, sizeof(_srv));

  DXGI_SWAP_CHAIN_DESC scd;
  memcpy(&scd, &g_interface->_scd, sizeof(DXGI_SWAP_CHAIN_DESC));
  scd.BufferDesc.Width = width;
  scd.BufferDesc.Height = height;
  scd.OutputWindow = (HWND)nwh;

  ID3D11Device* device = g_interface->_device;

  HRESULT hr;
  hr = g_interface->_factory->CreateSwapChain(device, &scd, &_swap_chain);
  if (FAILED(hr)) {
    c3_log("Failed to create swap chain.\n");
    exit(-1);
  }

  ID3D11Resource* ptr;
  DX_CHECK(_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&ptr));
  DX_CHECK(device->CreateRenderTargetView(ptr, NULL, &_rtv[0]));
  DX_RELEASE(ptr);

  DXGI_FORMAT fmt_dsv = is_depth(depth_format) ? s_textureFormat[depth_format].m_fmtDsv : DXGI_FORMAT_D24_UNORM_S8_UINT;
  D3D11_TEXTURE2D_DESC dsd;
  dsd.Width = scd.BufferDesc.Width;
  dsd.Height = scd.BufferDesc.Height;
  dsd.MipLevels = 1;
  dsd.ArraySize = 1;
  dsd.Format = fmt_dsv;
  dsd.SampleDesc = scd.SampleDesc;
  dsd.Usage = D3D11_USAGE_DEFAULT;
  dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  dsd.CPUAccessFlags = 0;
  dsd.MiscFlags = 0;

  ID3D11Texture2D* depthStencil;
  DX_CHECK(device->CreateTexture2D(&dsd, NULL, &depthStencil));
  DX_CHECK(device->CreateDepthStencilView(depthStencil, NULL, &_dsv));
  DX_RELEASE(depthStencil);

  _srv[0] = NULL;
  _dense_idx = dense_idx;
  _num = 1;
}

u16 FrameBufferD3D11::Destroy() {
  PreReset(true);

  DX_RELEASE(_swap_chain);

  _num = 0;
  _num_th = 0;

  u16 denseIdx = _dense_idx;
  _dense_idx = UINT16_MAX;

  return denseIdx;
}

void FrameBufferD3D11::PreReset(bool _force) {
  if (0 < _num_th || _force) {
    for (u32 ii = 0, num = _num; ii < num; ++ii) {
      DX_RELEASE(_srv[ii]);
      DX_RELEASE(_rtv[ii]);
    }

    DX_RELEASE(_dsv);
  }
}

void FrameBufferD3D11::PostReset() {
  _width = 0;
  _height = 0;

  if (0 < _num_th) {
    _num = 0;
    for (u32 ii = 0; ii < _num_th; ++ii) {
      TextureHandle handle = _th[ii];
      if (handle) {
        const TextureD3D11& texture = g_interface->_textures[handle.idx];

        if (0 == _width) {
          switch (texture._type) {
          case TextureD3D11::Texture2D:
          case TextureD3D11::TextureCube: {
            D3D11_TEXTURE2D_DESC desc;
            texture._texture2d->GetDesc(&desc);
            _width = desc.Width;
            _height = desc.Height;
          } break;

          case TextureD3D11::Texture3D: {
            D3D11_TEXTURE3D_DESC desc;
            texture._texture3d->GetDesc(&desc);
            _width = desc.Width;
            _height = desc.Height;
          } break;
          }
        }

        const u32 msaaQuality = ((texture._flags&C3_TEXTURE_RT_MSAA_MASK) >> C3_TEXTURE_RT_MSAA_SHIFT) - 1;
        const DXGI_SAMPLE_DESC& msaa = s_msaa[msaaQuality];

        if (is_depth((TextureFormat)texture._texture_format)) {
          if (_dsv) c3_log("Frame buffer already has depth-stencil attached.\n");

          switch (texture._type) {
          default:
          case TextureD3D11::Texture2D: {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = s_textureFormat[texture._texture_format].m_fmtDsv;
            dsvDesc.ViewDimension = 1 < msaa.Count
              ? D3D11_DSV_DIMENSION_TEXTURE2DMS
              : D3D11_DSV_DIMENSION_TEXTURE2D
              ;
            dsvDesc.Flags = 0;
            dsvDesc.Texture2D.MipSlice = 0;
            DX_CHECK(g_interface->_device->CreateDepthStencilView(texture._ptr, &dsvDesc, &_dsv));
          } break;

          case TextureD3D11::TextureCube: {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = s_textureFormat[texture._texture_format].m_fmtDsv;
            if (1 < msaa.Count) {
              dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
              dsvDesc.Texture2DMSArray.ArraySize = 1;
              dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
            } else {
              dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
              dsvDesc.Texture2DArray.ArraySize = 1;
              dsvDesc.Texture2DArray.FirstArraySlice = 0;
              dsvDesc.Texture2DArray.MipSlice = 0;
            }
            dsvDesc.Flags = 0;
            DX_CHECK(g_interface->_device->CreateDepthStencilView(texture._ptr, &dsvDesc, &_dsv));
          }break;
          }
        } else {
          switch (texture._type) {
          default:
          case TextureD3D11::Texture2D:
            DX_CHECK(g_interface->_device->CreateRenderTargetView(texture._ptr, NULL, &_rtv[_num]));
            break;

          case TextureD3D11::TextureCube: {
            D3D11_RENDER_TARGET_VIEW_DESC desc;
            desc.Format = s_textureFormat[texture._texture_format].m_fmt;
            if (1 < msaa.Count) {
              desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
              desc.Texture2DMSArray.ArraySize = 1;
              desc.Texture2DMSArray.FirstArraySlice = 0;
            } else {
              desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
              desc.Texture2DArray.ArraySize = 1;
              desc.Texture2DArray.FirstArraySlice = 0;
              desc.Texture2DArray.MipSlice = 0;
            }
            DX_CHECK(g_interface->_device->CreateRenderTargetView(texture._ptr, &desc, &_rtv[_num]));
          } break;

          case TextureD3D11::Texture3D: {
            D3D11_RENDER_TARGET_VIEW_DESC desc;
            desc.Format = s_textureFormat[texture._texture_format].m_fmt;
            desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipSlice = 0;
            desc.Texture3D.WSize = 1;
            desc.Texture3D.FirstWSlice = 0;
            DX_CHECK(g_interface->_device->CreateRenderTargetView(texture._ptr, &desc, &_rtv[_num]));
          } break;
          }

          ID3D11Resource* res = texture._ptr;
          if (texture._resolved_texture2d) res = texture._resolved_texture2d;
          DX_CHECK(g_interface->_device->CreateShaderResourceView(res, NULL, &_srv[_num]));
          _num++;
        }
      }
    }
  }
}

void FrameBufferD3D11::Resolve() {
  for (u8 i = 0; i < _num; ++i) {
    auto& texture = g_interface->_textures[_th[i].idx];
    texture.Resolve();
  }
}

void FrameBufferD3D11::Clear(const ViewClear& _clear, const float _palette[][4]) {
  ID3D11DeviceContext* context = g_interface->_context;

  if (C3_CLEAR_COLOR & _clear.flags) {
    if (C3_CLEAR_COLOR_USE_PALETTE & _clear.flags) {
      for (u32 ii = 0, num = _num; ii < num; ++ii) {
        u8 index = _clear.index[ii];
        if (NULL != _rtv[ii]
        && UINT8_MAX != index) {
          context->ClearRenderTargetView(_rtv[ii], _palette[index]);
        }
      }
    } else {
      float frgba[4] = {
        _clear.index[0] * 1.0f / 255.0f,
        _clear.index[1] * 1.0f / 255.0f,
        _clear.index[2] * 1.0f / 255.0f,
        _clear.index[3] * 1.0f / 255.0f,
      };
      for (u32 ii = 0, num = _num; ii < num; ++ii) {
        if (NULL != _rtv[ii]) {
          context->ClearRenderTargetView(_rtv[ii], frgba);
        }
      }
    }
  }

  if (NULL != _dsv
  && (C3_CLEAR_DEPTH | C3_CLEAR_STENCIL) & _clear.flags) {
    DWORD flags = 0;
    flags |= (_clear.flags & C3_CLEAR_DEPTH) ? D3D11_CLEAR_DEPTH : 0;
    flags |= (_clear.flags & C3_CLEAR_STENCIL) ? D3D11_CLEAR_STENCIL : 0;
    context->ClearDepthStencilView(_dsv, flags, _clear.depth, _clear.stencil);
  }
}
static D3D11_INPUT_ELEMENT_DESC* fillVertexDecl(D3D11_INPUT_ELEMENT_DESC* _out, const VertexDecl& _decl) {
  D3D11_INPUT_ELEMENT_DESC* elem = _out;

  for (u32 attr = 0; attr < VERTEX_ATTR_COUNT; ++attr) {
    if (UINT16_MAX != _decl.attributes[attr]) {
      memcpy(elem, &s_attrib[attr], sizeof(D3D11_INPUT_ELEMENT_DESC));

      if (0 == _decl.attributes[attr]) {
        elem->AlignedByteOffset = 0;
      } else {
        u8 num;
        DataType type;
        bool normalized;
        bool asInt;
        _decl.Query((VertexAttr)attr, num, type, normalized, asInt);
        elem->Format = s_attribType[type][num - 1][normalized];
        elem->AlignedByteOffset = _decl.offsets[attr];
      }

      ++elem;
    }
  }

  return elem;
}

GraphicsInterfaceD3D11::GraphicsInterfaceD3D11(GraphicsAPI api, int major_version, int minor_version)
: GraphicsInterface(api, major_version, minor_version) {

  g_interface = this;

  _initialized = false;

  _lost = 0;
  _num_windows = 0;
  memset(&_resolution, 0, sizeof(_resolution));

  _back_buffer_color = nullptr;
  _back_buffer_depth_stencil = nullptr;
  _current_color = nullptr;
  _current_depth_stencil = nullptr;

  _adapter = nullptr;
  _factory = nullptr;
  _device = nullptr;
  _context = nullptr;

  _wireframe = false;
  _rt_msaa = false;
  
  _vs_scratch = (u8*)c3_alloc(g_allocator, C3_MAX_CONSTANT_BUFER_SIZE, 16);
  _fs_scratch = (u8*)c3_alloc(g_allocator, C3_MAX_CONSTANT_BUFER_SIZE, 16);
  _vs_changes = 0;
  _fs_changes = 0;
  memset(_uniforms, 0, sizeof(_uniforms));

  _current_program = nullptr;
  _max_anisotropy = 1;
  _max_anisotropy_default = 8; // D3D11_REQ_MAXANISOTROPY
}

GraphicsInterfaceD3D11::~GraphicsInterfaceD3D11() {
  g_interface = nullptr;
  c3_free(g_allocator, _vs_scratch, 16);
  c3_free(g_allocator, _fs_scratch, 16);
}

void GraphicsInterfaceD3D11::Init() {

  HRESULT hr;

  hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&_factory);
  if (FAILED(hr)) {
    c3_log("Unable to create DXGI factory.\n");
    return;
  }

  // create a struct to hold information about the swap chain
  ZeroMemory(&_scd, sizeof(DXGI_SWAP_CHAIN_DESC));

  // fill the swap chain description struct
  _scd.BufferDesc.Width = C3_RESOLUTION_DEFAULT_WIDTH;
  _scd.BufferDesc.Height = C3_RESOLUTION_DEFAULT_HEIGHT;
  _scd.BufferDesc.RefreshRate.Numerator = 0;
  _scd.BufferDesc.RefreshRate.Denominator = 1;
  _scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  _scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  _scd.SampleDesc.Count = 1;
  _scd.SampleDesc.Quality = 0;
  _scd.BufferCount = 1;                                    // one back buffer
  _scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
  _scd.OutputWindow = (HWND)g_platform_data.nwh;           // the window to be used
  _scd.SampleDesc.Count = 1;                               // how many multisamples
  _scd.Windowed = true;                                    // windowed/full-screen mode

  // create a device, device context and swap chain using the information in the scd struct
  UINT flags = 0;
#ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL feature_levels[2] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
  hr = D3D11CreateDeviceAndSwapChain(NULL,
                                     D3D_DRIVER_TYPE_HARDWARE,
                                     NULL,
                                     flags,
                                     feature_levels,
                                     2,
                                     D3D11_SDK_VERSION,
                                     &_scd,
                                     &_swap_chain,
                                     &_device,
                                     &_feature_level,
                                     &_context);
  if (hr == E_INVALIDARG) {
    hr = D3D11CreateDeviceAndSwapChain(NULL,
                                       D3D_DRIVER_TYPE_HARDWARE,
                                       NULL,
                                       flags,
                                       feature_levels + 1,
                                       1,
                                       D3D11_SDK_VERSION,
                                       &_scd,
                                       &_swap_chain,
                                       &_device,
                                       &_feature_level,
                                       &_context);
  }
  if (FAILED(hr)) {
    c3_log("Failed to create d3d11 swap chain\n");
    return;
  }

  D3D11_FEATURE_DATA_THREADING feature_threading;
  hr = _device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &feature_threading, sizeof(feature_threading));
  if (SUCCEEDED(hr)) {
    c3_log("DriverConcurrentCreates support: %s.\n", feature_threading.DriverConcurrentCreates ? "YES" : "NO");
  }

  _user_defined_annotation = nullptr;
  hr = _context->QueryInterface(IID_PPV_ARGS(&_user_defined_annotation));
  if (FAILED(hr)) {
    c3_log("[C3] ID3DUserDefinedAnnotation query failed.\n");
    return;
  }

  _num_windows = 1;
  ConstantHandle handle;
  for (u32 ii = 0; ii < PREDEFINED_CONSTANT_COUNT; ++ii) {
    _uniform_reg.Add(handle, PredefinedConstant::TypeToName((PredefinedConstantType)ii), &_predefined_uniforms[ii]);
  }

  UpdateMsaa();
  PostReset();

  _initialized = true;

#ifdef REMOTERY_SUPPORT
  rmt_BindD3D11(_device, _context);
#endif
}

void GraphicsInterfaceD3D11::Shutdown() {
#ifdef REMOTERY_SUPPORT
  rmt_UnbindD3D11();
#endif
  PreReset();
  _context->ClearState();
  InvalidateCache();

  for (u32 ii = 0; ii < ARRAY_SIZE(_frame_buffers); ++ii) _frame_buffers[ii].Destroy();
  for (u32 ii = 0; ii < ARRAY_SIZE(_index_buffers); ++ii) _index_buffers[ii].Destroy();
  for (u32 ii = 0; ii < ARRAY_SIZE(_vertex_buffers); ++ii) _vertex_buffers[ii].Destroy();
  for (u32 ii = 0; ii < ARRAY_SIZE(_shaders); ++ii) _shaders[ii].Destroy();
  for (u32 ii = 0; ii < ARRAY_SIZE(_textures); ++ii) _textures[ii].Destroy();

  DX_RELEASE(_factory);
  DX_RELEASE(_swap_chain);
  DX_RELEASE(_context);
  DX_RELEASE(_device);
}

void GraphicsInterfaceD3D11::CreateIndexBuffer(IndexBufferHandle handle, const MemoryRegion* mem, u16 flags) {
  _index_buffers[handle.idx].Create(mem->size, mem->data, flags);
}

void GraphicsInterfaceD3D11::DestroyIndexBuffer(IndexBufferHandle handle) {
  _index_buffers[handle.idx].Destroy();
}

void GraphicsInterfaceD3D11::CreateVertexDecl(VertexDeclHandle handle, const VertexDecl& decl) {
  memcpy(&_vertex_decls[handle.idx], &decl, sizeof(VertexDecl));
}

void GraphicsInterfaceD3D11::DestroyVertexDecl(VertexDeclHandle handle) {}

void GraphicsInterfaceD3D11::CreateVertexBuffer(VertexBufferHandle handle, const MemoryRegion* mem, VertexDeclHandle decl_handle, u16 flags) {
  _vertex_buffers[handle.idx].Create(mem->size, mem->data, decl_handle, flags);
}

void GraphicsInterfaceD3D11::DestroyVertexBuffer(VertexBufferHandle handle) {
  _vertex_buffers[handle.idx].Destroy();
}

void GraphicsInterfaceD3D11::CreateDynamicIndexBuffer(IndexBufferHandle handle, u32 size, u16 flags) {
  _index_buffers[handle.idx].Create(size, nullptr, flags);
}

void GraphicsInterfaceD3D11::UpdateDynamicIndexBuffer(IndexBufferHandle handle, u32 offset, u32 size, const MemoryRegion* mem) {
  _index_buffers[handle.idx].Update(offset, min(size, mem->size), mem->data);
}

void GraphicsInterfaceD3D11::CreateDynamicVertexBuffer(VertexBufferHandle handle, u32 size, u16 flags) {
  _vertex_buffers[handle.idx].Create(size, nullptr, VertexDeclHandle(), flags);
}

void GraphicsInterfaceD3D11::UpdateDynamicVertexBuffer(VertexBufferHandle handle, u32 offset, u32 size, const MemoryRegion* mem) {
  _vertex_buffers[handle.idx].Update(offset, min(size, mem->size), mem->data);
}

void GraphicsInterfaceD3D11::CreateShader(ShaderHandle handle, const MemoryRegion* mem) {
  _shaders[handle.idx].Create(mem);
}

void GraphicsInterfaceD3D11::DestroyShader(ShaderHandle handle) {
  _shaders[handle.idx].Destroy();
}

void GraphicsInterfaceD3D11::CreateProgram(ProgramHandle handle, ShaderHandle vsh, ShaderHandle fsh) {
  _programs[handle.idx].Create(&_shaders[vsh.idx], fsh ? &_shaders[fsh.idx] : nullptr);
}

void GraphicsInterfaceD3D11::DestroyProgram(ProgramHandle handle) {
  _programs[handle.idx].Destroy();
}

void GraphicsInterfaceD3D11::CreateTexture(TextureHandle handle, const MemoryRegion* mem, u32 flags, u8 skip) {
  _textures[handle.idx].Create(mem, flags, skip);
}

void GraphicsInterfaceD3D11::UpdateTextureBegin(TextureHandle handle, u8 side, u8 mip) {}

void GraphicsInterfaceD3D11::UpdateTexture(TextureHandle handle, u8 side, u8 mip, const TextureRect& rect, u16 z, u16 depth, u16 pitch, const MemoryRegion* mem) {
  _textures[handle.idx].Update(side, mip, rect, z, depth, pitch, mem);
}

void GraphicsInterfaceD3D11::UpdateTextureEnd() {}

void GraphicsInterfaceD3D11::ResizeTexture(TextureHandle handle, u16 width, u16 height) {
  TextureD3D11& texture = _textures[handle.idx];

  BlobWriter stream;
  stream.Reserve(4 + sizeof(TextureCreate));
  u32 magic = C3_CHUNK_MAGIC_TEX;
  stream.Write(magic);

  TextureCreate tc;
  tc.flags = texture._flags;
  tc.width = width;
  tc.height = height;
  tc.sides = 0;
  tc.depth = 0;
  tc.num_mips = 1;
  tc.format = texture._requested_format;
  tc.cube_map = false;
  tc.mem = NULL;
  stream.Write(tc);

  texture.Destroy();
  texture.Create(mem_copy(stream.GetData(), stream.GetCapacity()), tc.flags, 0);
}

void GraphicsInterfaceD3D11::DestroyTexture(TextureHandle handle) {
  _textures[handle.idx].Destroy();
}

void GraphicsInterfaceD3D11::CreateFrameBuffer(FrameBufferHandle handle, u8 num, const TextureHandle* texture_handles) {
  _frame_buffers[handle.idx].Create(num, texture_handles);
}

void GraphicsInterfaceD3D11::CreateFrameBuffer(FrameBufferHandle handle, void* nwh, u32 width, u32 height, TextureFormat depth_format) {
  u16 denseIdx = _num_windows++;
  _windows[denseIdx] = handle;
  _frame_buffers[handle.idx].Create(denseIdx, nwh, width, height, depth_format);
}

void GraphicsInterfaceD3D11::DestroyFrameBuffer(FrameBufferHandle handle) {
  _frame_buffers[handle.idx].Destroy();
}

void GraphicsInterfaceD3D11::CreateConstant(ConstantHandle handle, ConstantType type, u16 num, stringid name) {
  if (_uniforms[handle.idx]) C3_FREE(g_allocator, _uniforms[handle.idx]);

  u32 size = ALIGN_16(CONSTANT_TYPE_SIZE[type] * num);
  void* data = C3_ALLOC(g_allocator, size);
  memset(data, 0, size);
  _uniforms[handle.idx] = data;
  _uniform_reg.Add(handle, name, data);
}

void GraphicsInterfaceD3D11::DestroyConstant(ConstantHandle handle) {
  C3_FREE(g_allocator, _uniforms[handle.idx]);
  _uniforms[handle.idx] = NULL;
}

void GraphicsInterfaceD3D11::UpdateConstant(u16 loc, const void* data, u32 size) {
  memcpy(_uniforms[loc], data, size);
}

void GraphicsInterfaceD3D11::SetMarker(const char* marker, u32 size) {
  if (_user_defined_annotation) {
    static WCHAR wstr[1024];
    MultiByteToWideChar(CP_UTF8, 0, marker, size, wstr, sizeof(wstr));
    _user_defined_annotation->SetMarker(wstr);
  }
}

void GraphicsInterfaceD3D11::Submit(RenderFrame* render, ClearQuad& clear_quad) {
#ifdef REMOTERY_SUPPORT
  GPU_PROFILE_BLOCK(Submit);
  PROFILE_BLOCK(Submit);
#endif
  ID3D11DeviceContext* context = _context;

  UpdateResolution(render->resolution);

  if (render->ib_offset > 0) {
    TransientIndexBuffer* ib = render->transient_ib;
    _index_buffers[ib->handle.idx].Update(0, render->ib_offset, ib->data);
  }

  if (render->vb_offset > 0) {
    TransientVertexBuffer* vb = render->transient_vb;
    _vertex_buffers[vb->handle.idx].Update(0, render->vb_offset, vb->data);
  }

  render->Sort();

  RenderItem currentState;
  currentState.Clear();
  currentState.flags = C3_STATE_NONE;
  //currentState.stencil = packStencil(C3_STENCIL_NONE, C3_STENCIL_NONE);

  ViewState& viewState = _view_state;
  viewState.Reset(render, false);

  //bool wireframe = !!(render->m_debug &C3_DEBUG_WIREFRAME);
  bool wireframe = false;
  bool scissorEnabled = false;
  SetDebugWireframe(wireframe);

  u16 programIdx = UINT16_MAX;
  SortKey key;
  u16 view = UINT16_MAX;
  FrameBufferHandle fbh;

  const u64 primType = 0;
  u8 primIndex = u8(primType >> C3_STATE_PT_SHIFT);
  PrimInfo prim = PRIM_INFO[primIndex];
  context->IASetPrimitiveTopology(prim.type);

  bool viewHasScissor = false;
  Rect viewScissorRect;

  // reset the framebuffer to be the backbuffer; depending on the swap effect,
  // if we don't do this we'll only see one frame of output and then nothing
  SetFrameBuffer(fbh);

  u8 eye = 0;
  viewState._rect = render->rect[0];

  i32 numItems = render->render_item_count;
  for (i32 item = 0, restartItem = numItems; item < numItems || restartItem < numItems;) {
    key.Decode(render->sort_keys[item], render->view_remap);
    const bool viewChanged = key.view != view;

    const RenderItem& draw = render->render_items[render->sort_values[item]];
    ++item;

    if (viewChanged) {
#ifdef REMOTERY_SUPPORT
      if (item > 1) {
        END_PROFILE_BLOCK();
        END_GPU_PROFILE_BLOCK();
      }
      BEGIN_PROFILE_BLOCK_DYNAMIC(_view_names[key.view]);
      BEGIN_GPU_PROFILE_BLOCK_DYNAMIC(_view_names[key.view]);
#endif
      view = key.view;
      programIdx = UINT16_MAX;

      if (render->fb[view].idx != fbh.idx) {
        fbh = render->fb[view];
        SetFrameBuffer(fbh);
      }

      eye = 0;

      viewState._rect = render->rect[view];

      const Rect& scissorRect = render->scissor[view];
      viewHasScissor = scissorRect.right > 0 || scissorRect.bottom > 0;
      viewScissorRect = viewHasScissor ? scissorRect : viewState._rect;

      D3D11_VIEWPORT vp;
      float height = fbh ? (float)_frame_buffers[fbh.idx]._height : _resolution.height;
      vp.TopLeftX = (float)viewState._rect.left;
      vp.TopLeftY = (float)height - viewState._rect.top;
      vp.Width = (float)viewState._rect.Width();
      vp.Height = (float)viewState._rect.Height();
      vp.MinDepth = 0.0f;
      vp.MaxDepth = 1.0f;
      context->RSSetViewports(1, &vp);
      ViewClear& clr = render->view_clear[view];

      if (C3_CLEAR_NONE != (clr.flags & C3_CLEAR_MASK)) {
        Clear(clear_quad, viewState._rect, clr, render->color_palette);
        prim = PRIM_INFO[ARRAY_SIZE(PRIM_INFO) - 1]; // Force primitive type update after clear quad.
      }
    }

    bool resetState = viewChanged;

    const u64 newFlags = draw.flags;
    u64 changedFlags = currentState.flags ^ draw.flags;
    changedFlags |= currentState.rgba != draw.rgba ? C3_D3D11_BLEND_STATE_MASK : 0;
    currentState.flags = newFlags;

    if (resetState) {
      currentState.Clear();
      currentState.scissor = !draw.scissor;
      changedFlags = C3_STATE_MASK;
      currentState.flags = newFlags;

      SetBlendState(newFlags);
      SetDepthStencilState(newFlags);

      const u64 pt = newFlags & C3_STATE_PT_MASK;
      primIndex = u8(pt >> C3_STATE_PT_SHIFT);
    }

    if (prim.type != PRIM_INFO[primIndex].type) {
      prim = PRIM_INFO[primIndex];
      context->IASetPrimitiveTopology(prim.type);
    }

    u16 scissor = draw.scissor;
    if (currentState.scissor != scissor) {
      currentState.scissor = scissor;

      if (UINT16_MAX == scissor) {
        scissorEnabled = viewHasScissor;
        if (viewHasScissor) {
          D3D11_RECT rc;
          rc.left = viewScissorRect.left;
          rc.top = viewScissorRect.bottom;
          rc.right = viewScissorRect.right;
          rc.bottom = viewScissorRect.top;
          context->RSSetScissorRects(1, &rc);
        }
      } else {
        Rect scissorRect = render->rect_cache._cache[scissor];
        // TODO: intersect scissor rect
        //scissorRect.Intersect(viewScissorRect, render->rect_cache._cache[scissor]);
        scissorEnabled = true;
        u16 height = fbh ? (u16)_frame_buffers[fbh.idx]._height : _resolution.height;
        D3D11_RECT rc;
        rc.left = scissorRect.left;
        rc.top = height - scissorRect.top;
        rc.right = scissorRect.right;
        rc.bottom = height - scissorRect.bottom;
        context->RSSetScissorRects(1, &rc);
      }

      SetRasterizerState(newFlags, wireframe, scissorEnabled);
    }

    if (C3_D3D11_DEPTH_STENCIL_MASK & changedFlags) {
      SetDepthStencilState(newFlags);
    }

    if (C3_D3D11_BLEND_STATE_MASK & changedFlags) {
      SetBlendState(newFlags, draw.rgba);
      currentState.rgba = draw.rgba;
    }

    if ((C3_STATE_CULL_MASK | C3_STATE_ALPHA_REF_MASK | C3_STATE_PT_MASK | C3_STATE_POINT_SIZE_MASK | C3_STATE_MSAA) & changedFlags) {
      if ((C3_STATE_CULL_MASK | C3_STATE_MSAA) & changedFlags) SetRasterizerState(newFlags, wireframe, scissorEnabled);

      if (C3_STATE_ALPHA_REF_MASK & changedFlags) {
        u32 ref = (newFlags&C3_STATE_ALPHA_REF_MASK) >> C3_STATE_ALPHA_REF_SHIFT;
        viewState._alpha_ref = ref / 255.0f;
      }

      const u64 pt = newFlags&C3_STATE_PT_MASK;
      primIndex = u8(pt >> C3_STATE_PT_SHIFT);
      if (prim.type != PRIM_INFO[primIndex].type) {
        prim = PRIM_INFO[primIndex];
        context->IASetPrimitiveTopology(prim.type);
      }
    }

    bool programChanged = false;
    bool constantsChanged = draw.constant_begin < draw.constant_end;

    UpdateConstants(render->constant_buffer, draw.constant_begin, draw.constant_end);

    if (key.program != programIdx) {
      programIdx = key.program;

      if (UINT16_MAX == programIdx) {
        _current_program = NULL;

        context->VSSetShader(NULL, NULL, 0);
        context->PSSetShader(NULL, NULL, 0);
      } else {
        ProgramD3D11& program = _programs[programIdx];
        _current_program = &program;

        const ShaderD3D11* vsh = program._vsh;
        context->VSSetShader(vsh->_vertex_shader, NULL, 0);
        context->VSSetConstantBuffers(0, 1, &vsh->_buffer);

        const ShaderD3D11* fsh = program._fsh;
        if (NULL != _current_color
        || fsh->_has_depth_op) {
          context->PSSetShader(fsh->_pixel_shader, NULL, 0);
          context->PSSetConstantBuffers(0, 1, &fsh->_buffer);
        } else {
          context->PSSetShader(NULL, NULL, 0);
        }
      }

      programChanged =
        constantsChanged = true;
    }

    if (UINT16_MAX != programIdx) {
      ProgramD3D11& program = _programs[programIdx];

      if (constantsChanged) {
        ConstantBuffer* vcb = program._vsh->_constant_buffer;
        if (vcb) Commit(*vcb);

        ConstantBuffer* fcb = program._fsh->_constant_buffer;
        if (fcb) Commit(*fcb);
      }

      viewState.SetPredefined<4>(this, view, eye, program, render, draw);

      if (constantsChanged || program._num_predefined > 0) CommitShaderConstants();

      u32 changes = 0;
      for (u8 stage = 0; stage < MAX_RENDER_ITEM_BINDING_COUNT; ++stage) {
        const Binding& bind = draw.bind[stage];
        Binding& current = currentState.bind[stage];
        if (current.idx != bind.idx || current.flags != bind.flags || programChanged) {
          if (UINT16_MAX != bind.idx) {
            TextureD3D11& texture = _textures[bind.idx];
            texture.Commit(stage, bind.flags, render->color_palette);
          } else {
            _texture_stage._srv[stage] = NULL;
            _texture_stage._sampler[stage] = NULL;
          }

          ++changes;
        }

        current = bind;
      }

      if (changes > 0) CommitTextureStage();

      if (programChanged || currentState.vertex_decl.idx != draw.vertex_decl.idx ||
          currentState.vertex_buffer.idx != draw.vertex_buffer.idx ||
          //currentState.instance_data_buffer.idx != draw.instance_data_buffer.idx ||
          currentState.instance_data_offset != draw.instance_data_offset ||
          currentState.instance_data_stride != draw.instance_data_stride) {
        currentState.vertex_decl = draw.vertex_decl;
        currentState.vertex_buffer = draw.vertex_buffer;
        //currentState.instance_data_buffer.idx = draw.instance_data_buffer.idx;
        currentState.instance_data_offset = draw.instance_data_offset;
        currentState.instance_data_stride = draw.instance_data_stride;

        u32 handle = draw.vertex_buffer.idx;
        if (UINT16_MAX != handle) {
          const VertexBufferD3D11& vb = _vertex_buffers[handle];

          u32 decl = vb._decl ? vb._decl.idx : draw.vertex_decl.idx;
          const VertexDecl& vertexDecl = _vertex_decls[decl];
          u32 stride = vertexDecl.stride;
          u32 offset = 0;
          context->IASetVertexBuffers(0, 1, &vb._ptr, &stride, &offset);

#if 0
          if (draw.instance_data_buffer) {
            const VertexBufferD3D11& inst = _vertex_buffers[draw.instance_data_buffer.idx];
            u32 instance_stride = draw.instance_data_stride;
            context->IASetVertexBuffers(1, 1, &inst._ptr, &instance_stride, &draw.instance_data_offset);
            SetInputLayout(vertexDecl, program, draw.instance_data_stride / 16);
          } else {
#endif
            context->IASetVertexBuffers(1, 1, s_zero.m_buffer, s_zero.m_zero, s_zero.m_zero);
            SetInputLayout(vertexDecl, program, 0);
#if 0
          }
#endif
        } else {
          context->IASetVertexBuffers(0, 1, s_zero.m_buffer, s_zero.m_zero, s_zero.m_zero);
        }
      }

      if (currentState.index_buffer.idx != draw.index_buffer.idx) {
        currentState.index_buffer = draw.index_buffer;

        u32 handle = draw.index_buffer.idx;
        if (UINT16_MAX != handle) {
          const IndexBufferD3D11& ib = _index_buffers[handle];
          context->IASetIndexBuffer(ib._ptr,
                                      (ib._flags & C3_BUFFER_INDEX32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT,
                                      0);
        } else {
          context->IASetIndexBuffer(NULL, DXGI_FORMAT_R16_UINT, 0);
        }
      }

      if (currentState.vertex_buffer) {
        u32 numVertices = draw.num_vertices;
        if (UINT32_MAX == numVertices) {
          const VertexBufferD3D11& vb = _vertex_buffers[currentState.vertex_buffer.idx];
          u32 decl = vb._decl ? vb._decl.idx : draw.vertex_decl.idx;
          const VertexDecl& vertexDecl = _vertex_decls[decl];
          numVertices = vb._size / vertexDecl.stride;
        }

        u32 numIndices = 0;

        if (draw.index_buffer) {
          if (UINT32_MAX == draw.num_indices) {
            const IndexBufferD3D11& ib = _index_buffers[draw.index_buffer.idx];
            const u32 indexSize = 0 == (ib._flags & C3_BUFFER_INDEX32) ? 2 : 4;
            numIndices = ib._size / indexSize;

            if (draw.num_instances > 1) context->DrawIndexedInstanced(numIndices, draw.num_instances, 0, draw.start_vertex, 0);
            else context->DrawIndexed(numIndices, 0, draw.start_vertex);
          } else if (prim.min <= draw.num_indices) {
            numIndices = draw.num_indices;
            if (draw.num_instances > 1) context->DrawIndexedInstanced(numIndices, draw.num_instances, draw.start_index, draw.start_vertex, 0);
            else context->DrawIndexed(numIndices, draw.start_index, draw.start_vertex);
          }
        } else {
          if (draw.num_instances > 1) context->DrawInstanced(numVertices, draw.num_instances, draw.start_vertex, 0);
          else context->Draw(numVertices, draw.start_vertex);
        }
      }
    }
  }
#ifdef REMOTERY_SUPPORT
  if (numItems > 0) {
    END_PROFILE_BLOCK();
    END_GPU_PROFILE_BLOCK();
  }
#endif
}

void GraphicsInterfaceD3D11::Flip() {
  HRESULT hr = S_OK;
  u32 syncInterval = !!(_resolution.flags & C3_RESET_VSYNC);

  for (u32 ii = 1, num = _num_windows; ii < num && SUCCEEDED(hr); ++ii) {
    hr = _frame_buffers[_windows[ii].idx]._swap_chain->Present(syncInterval, 0);
  }

  if (SUCCEEDED(hr)) {
    hr = _swap_chain->Present(syncInterval, DXGI_PRESENT_DO_NOT_WAIT);
    while (hr == DXGI_ERROR_WAS_STILL_DRAWING) {
      JobScheduler::Instance()->Yield();
      hr = _swap_chain->Present(syncInterval, DXGI_PRESENT_DO_NOT_WAIT);
    }
  }

  if (FAILED(hr) && is_lost(hr)) {
    ++_lost;
    if (_lost >= 10) {
      c3_log("Device is lost. FAILED 0x%08x\n", hr);
      exit(-1);
    }
  } else {
    _lost = 0;
  }
}

void GraphicsInterfaceD3D11::SaveScreenshot(const String& path) {}

void GraphicsInterfaceD3D11::_SetConstant(u8 flags, u32 loc, const void* val, u32 num) {
  if (flags & CONSTANT_FRAGMENTBIT) {
    memcpy(&_fs_scratch[loc], val, num * 4);
    _fs_changes += num;
    //_fs_changes = max(_fs_changes, loc + num * 4);
  } else {
    memcpy(&_vs_scratch[loc], val, num * 4);
    _vs_changes += num;
    //_vs_changes = max(_vs_changes, loc + num * 4);
  }
}

void GraphicsInterfaceD3D11::SetFrameBuffer(FrameBufferHandle fbh, bool msaa) {
  if (_fbh && _fbh.idx != fbh.idx &&  _rt_msaa) {
    FrameBufferD3D11& frameBuffer = _frame_buffers[_fbh.idx];
    frameBuffer.Resolve();
  }

  if (!fbh) {
    _context->OMSetRenderTargets(1, &_back_buffer_color, _back_buffer_depth_stencil);

    _current_color = _back_buffer_color;
    _current_depth_stencil = _back_buffer_depth_stencil;
  } else {
    InvalidateTextureStage();

    FrameBufferD3D11& frameBuffer = _frame_buffers[fbh.idx];
    _context->OMSetRenderTargets(frameBuffer._num, frameBuffer._rtv, frameBuffer._dsv);

    _current_color = (frameBuffer._num > 0) ? frameBuffer._rtv[0] : nullptr;
    _current_depth_stencil = frameBuffer._dsv;
  }

  _fbh = fbh;
  _rt_msaa = msaa;
}

void GraphicsInterfaceD3D11::CommitShaderConstants() {
  D3D11_MAPPED_SUBRESOURCE mapped_res;
  if (_vs_changes > 0) {
    if (_current_program->_vsh->_buffer) {
      _context->Map(_current_program->_vsh->_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
      memcpy(mapped_res.pData, _vs_scratch, _current_program->_vsh->_byte_width);
      _context->Unmap(_current_program->_vsh->_buffer, 0);
    }
    _vs_changes = 0;
  }

  if (_fs_changes > 0) {
    if (_current_program->_fsh->_buffer) {
      _context->Map(_current_program->_fsh->_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
      memcpy(mapped_res.pData, _fs_scratch, _current_program->_fsh->_byte_width);
      _context->Unmap(_current_program->_fsh->_buffer, 0);
    }
    _fs_changes = 0;
  }
}

void GraphicsInterfaceD3D11::Commit(ConstantBuffer& _uniformBuffer) {
  _uniformBuffer.Reset();

  for (;;) {
    u32 opcode = _uniformBuffer.Read();

    if (opcode == CONSTANT_END) break;

    ConstantType type;
    u16 loc;
    u16 num;
    u16 copy;
    ConstantBuffer::DecodeOpcode(opcode, type, loc, num, copy);

    const char* data;
    if (copy) {
      data = _uniformBuffer.Read(CONSTANT_TYPE_SIZE[type] * num);
    } else {
      ConstantHandle handle;
      memcpy(&handle, _uniformBuffer.Read(sizeof(ConstantHandle)), sizeof(ConstantHandle));
      data = (const char*)_uniforms[handle.idx];
    }

#define CASE_IMPLEMENT_UNIFORM(_uniform, _dxsuffix, _type, _elem) \
		case CONSTANT_##_uniform: \
		case CONSTANT_##_uniform | CONSTANT_FRAGMENTBIT: { \
		  _SetConstant(u8(type), loc, data, num * _elem); \
    } break;

    switch ((u32)type) {
    case CONSTANT_MAT3:
    case CONSTANT_MAT3 | CONSTANT_FRAGMENTBIT: {
      float* f = (float*)data;
      float4x4 mat4(f[0], f[1], f[2], 0.f,
                    f[3], f[4], f[5], 0.f,
                    f[6], f[7], f[8], 0.f, 
                    0.f, 0.f, 0.f, 0.f);
#if 0
      for (u32 ii = 0, count = num / 3; ii < count; ++ii, loc += 3 * 16, value += 9) {
        float4x4 mtx(value[0], value[1], value[2], 0.f,
                    value[3], value[4], value[5], 0.f,
                    value[6], value[7], value[8], 0.f,
                    0.f, 0.f, 0.f, 0.f);
        _SetConstant(u8(type), loc, mtx.GetBuffer(), 3);
      }
#endif
      _SetConstant(u8(type), loc, mat4.ptr(), num * 16);
    } break;
    
    CASE_IMPLEMENT_UNIFORM(BOOL32, I, int, 1);
    CASE_IMPLEMENT_UNIFORM(INT, I, int, 1);
    CASE_IMPLEMENT_UNIFORM(FLOAT, I, float, 1);
    CASE_IMPLEMENT_UNIFORM(VEC2, F, float, 2);
    CASE_IMPLEMENT_UNIFORM(VEC3, F, float, 3);
    CASE_IMPLEMENT_UNIFORM(VEC4, F, float, 4);
    CASE_IMPLEMENT_UNIFORM(MAT4, F, float, 16);

    case CONSTANT_END:
      break;

    default:
      c3_log("[C3] %4d: INVALID 0x%08x, t %d, l %d, n %d, c %d\n", _uniformBuffer.GetPos(), opcode, type, loc, num, copy);
      break;
    }
#undef CASE_IMPLEMENT_UNIFORM
  }
}

void GraphicsInterfaceD3D11::Clear(const ViewClear& view_clear, const float palette[][4]) {
  if (_fbh) {
    FrameBufferD3D11& frameBuffer = _frame_buffers[_fbh.idx];
    frameBuffer.Clear(view_clear, palette);
  } else {
    if (NULL != _current_color &&  C3_CLEAR_COLOR & view_clear.flags) {
      if (C3_CLEAR_COLOR_USE_PALETTE & view_clear.flags) {
        u8 index = view_clear.index[0];
        if (UINT8_MAX != index) _context->ClearRenderTargetView(_current_color, palette[index]);
      } else {
        float frgba[4] = {
          view_clear.index[0] * 1.0f / 255.0f,
          view_clear.index[1] * 1.0f / 255.0f,
          view_clear.index[2] * 1.0f / 255.0f,
          view_clear.index[3] * 1.0f / 255.0f,
        };
        _context->ClearRenderTargetView(_current_color, frgba);
      }
    }

    if (NULL != _current_depth_stencil && (C3_CLEAR_DEPTH | C3_CLEAR_STENCIL) & view_clear.flags) {
      DWORD flags = 0;
      flags |= (view_clear.flags & C3_CLEAR_DEPTH) ? D3D11_CLEAR_DEPTH : 0;
      flags |= (view_clear.flags & C3_CLEAR_STENCIL) ? D3D11_CLEAR_STENCIL : 0;
      _context->ClearDepthStencilView(_current_depth_stencil, flags, view_clear.depth, view_clear.stencil);
    }
  }
}

void GraphicsInterfaceD3D11::Clear(ClearQuad& clear_quad, const Rect& rect, const ViewClear& view_clear, const float palette[][4]) {
  u32 width;
  u32 height;

  if (_fbh) {
    const FrameBufferD3D11& fb = _frame_buffers[_fbh.idx];
    width = fb._width;
    height = fb._height;
  } else {
    width = _scd.BufferDesc.Width;
    height = _scd.BufferDesc.Height;
  }

  if (0 == rect.left && 0 == rect.bottom && (int)width == rect.right && (int)height == rect.top) {
    Clear(view_clear, palette);
  } else {
    ID3D11DeviceContext* context = _context;

    u64 state = 0;
    state |= view_clear.flags & C3_CLEAR_COLOR ? C3_STATE_RGB_WRITE | C3_STATE_ALPHA_WRITE : 0;
    state |= view_clear.flags & C3_CLEAR_DEPTH ? C3_STATE_DEPTH_TEST_ALWAYS | C3_STATE_DEPTH_WRITE : 0;

    u64 stencil = 0;
    stencil |= view_clear.flags & C3_CLEAR_STENCIL ? 0
      | C3_STENCIL_TEST_ALWAYS
      | C3_STENCIL_FUNC_REF(view_clear.stencil)
      | C3_STENCIL_FUNC_RMASK(0xff)
      | C3_STENCIL_OP_FAIL_S_REPLACE
      | C3_STENCIL_OP_FAIL_Z_REPLACE
      | C3_STENCIL_OP_PASS_Z_REPLACE
      : 0
      ;

    SetBlendState(state);
    SetDepthStencilState(state, stencil);
    SetRasterizerState(state);

    u32 numMrt = 1;
    FrameBufferHandle fbh = _fbh;
    if (fbh) {
      const FrameBufferD3D11& fb = _frame_buffers[fbh.idx];
      numMrt = max<u32>(1, fb._num);
    }

    ProgramD3D11& program = _programs[clear_quad.program[numMrt - 1].idx];
    _current_program = &program;
    context->VSSetShader(program._vsh->_vertex_shader, NULL, 0);
    context->VSSetConstantBuffers(0, 1, s_zero.m_buffer);
    if (NULL != _current_color) {
      const ShaderD3D11* fsh = program._fsh;
      context->PSSetShader(fsh->_pixel_shader, NULL, 0);
      context->PSSetConstantBuffers(0, 1, &fsh->_buffer);

      if (C3_CLEAR_COLOR_USE_PALETTE & view_clear.flags) {
        float mrtClear[COLOR_ATTACHMENT_COUNT][4];
        for (u32 ii = 0; ii < numMrt; ++ii) {
          u8 index = min<u8>(C3_MAX_COLOR_PALETTE - 1, view_clear.index[ii]);
          memcpy(mrtClear[ii], palette[index], 16);
        }

        context->UpdateSubresource(fsh->_buffer, 0, 0, mrtClear, 0, 0);
      } else {
        float rgba[4] = {
          view_clear.index[0] * 1.0f / 255.0f,
          view_clear.index[1] * 1.0f / 255.0f,
          view_clear.index[2] * 1.0f / 255.0f,
          view_clear.index[3] * 1.0f / 255.0f,
        };

        context->UpdateSubresource(fsh->_buffer, 0, 0, rgba, 0, 0);
      }
    } else {
      context->PSSetShader(NULL, NULL, 0);
    }

    VertexBufferD3D11& vb = _vertex_buffers[clear_quad.vb->handle.idx];
    const VertexDecl& vertexDecl = _vertex_decls[clear_quad.vb->decl.idx];
    const u32 stride = vertexDecl.stride;
    const u32 offset = 0;

    {
      struct Vertex {
        float m_x;
        float m_y;
        float m_z;
      };

      Vertex* vertex = (Vertex*)clear_quad.vb->data;
      if (stride != sizeof(Vertex)) c3_log("Stride/Vertex mismatch (stride %d, sizeof(Vertex) %d)\n", stride, sizeof(Vertex));

      const float depth = view_clear.depth;

      vertex->m_x = -1.0f;
      vertex->m_y = -1.0f;
      vertex->m_z = depth;
      vertex++;
      vertex->m_x = 1.0f;
      vertex->m_y = -1.0f;
      vertex->m_z = depth;
      vertex++;
      vertex->m_x = -1.0f;
      vertex->m_y = 1.0f;
      vertex->m_z = depth;
      vertex++;
      vertex->m_x = 1.0f;
      vertex->m_y = 1.0f;
      vertex->m_z = depth;
    }

    _vertex_buffers[clear_quad.vb->handle.idx].Update(0, 4 * clear_quad.decl.stride, clear_quad.vb->data);
    context->IASetVertexBuffers(0, 1, &vb._ptr, &stride, &offset);
    SetInputLayout(vertexDecl, program, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);
  }
}

void GraphicsInterfaceD3D11::SetInputLayout(const VertexDecl& vertex_decl, const ProgramD3D11& program, u16 num_instance_data) {
  u64 layoutHash = (u64(vertex_decl.hash) << 32) | program._vsh->_hash;
  layoutHash ^= num_instance_data;
  ID3D11InputLayout* layout = _input_layout_cache.Find(layoutHash);
  if (NULL == layout) {
    D3D11_INPUT_ELEMENT_DESC vertexElements[VERTEX_ATTR_COUNT + INSTANCE_ATTR_COUNT + 1];

    VertexDecl decl;
    memcpy(&decl, &vertex_decl, sizeof(VertexDecl));
#if 1
    const u16* attrMask = program._vsh->_attr_mask;

    for (u32 ii = 0; ii < VERTEX_ATTR_COUNT; ++ii) {
      u16 mask = attrMask[ii];
      u16 attr = (decl.attributes[ii] & mask);
      decl.attributes[ii] = attr == 0 ? UINT16_MAX : attr == UINT16_MAX ? 0 : attr;
    }
#endif
    D3D11_INPUT_ELEMENT_DESC* elem = fillVertexDecl(vertexElements, decl);
    u32 num = u32(elem - vertexElements);

    const D3D11_INPUT_ELEMENT_DESC inst = {"DATA", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1};

    for (u32 ii = 0; ii < INSTANCE_ATTR_COUNT; ++ii) {
      if (attrMask[ii + VERTEX_ATTR_COUNT]) {
        memcpy(elem, &inst, sizeof(D3D11_INPUT_ELEMENT_DESC));
        elem->InputSlot = 1;
        elem->SemanticIndex = ii;
        elem->AlignedByteOffset = ii * 16;
        ++elem;
      }
    }

    num = u32(elem - vertexElements);
    DX_CHECK(_device->CreateInputLayout(vertexElements
      , num
      , program._vsh->_code->data
      , program._vsh->_code->size
      , &layout
      ));
    _input_layout_cache.Add(layoutHash, layout);
  }

  _context->IASetInputLayout(layout);
}

void GraphicsInterfaceD3D11::SetBlendState(u64 state, u32 rgba) {
  state &= C3_D3D11_BLEND_STATE_MASK;

  Hasher murmur;
  murmur.Begin();
  murmur.Add(state);

  const u64 f0 = C3_STATE_BLEND_FUNC(C3_STATE_BLEND_FACTOR, C3_STATE_BLEND_FACTOR);
  const u64 f1 = C3_STATE_BLEND_FUNC(C3_STATE_BLEND_INV_FACTOR, C3_STATE_BLEND_INV_FACTOR);
  bool hasFactor = f0 == (state & f0)
    || f1 == (state & f1)
    ;

  float blendFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  if (hasFactor) {
    blendFactor[0] = ((rgba >> 24)) / 255.0f;
    blendFactor[1] = ((rgba >> 16) & 0xff) / 255.0f;
    blendFactor[2] = ((rgba >> 8) & 0xff) / 255.0f;
    blendFactor[3] = ((rgba)& 0xff) / 255.0f;
  } else {
    murmur.Add(rgba);
  }

  u32 hash = murmur.End();

  ID3D11BlendState* bs = _blend_state_cache.Find(hash);
  if (NULL == bs) {
    D3D11_BLEND_DESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.IndependentBlendEnable = !!(C3_STATE_BLEND_INDEPENDENT & state);

    D3D11_RENDER_TARGET_BLEND_DESC* drt = &desc.RenderTarget[0];
    drt->BlendEnable = !!(C3_STATE_BLEND_MASK & state);

    const u32 blend = u32((state&C3_STATE_BLEND_MASK) >> C3_STATE_BLEND_SHIFT);
    const u32 equation = u32((state&C3_STATE_BLEND_EQUATION_MASK) >> C3_STATE_BLEND_EQUATION_SHIFT);

    const u32 srcRGB = (blend)& 0xf;
    const u32 dstRGB = (blend >> 4) & 0xf;
    const u32 srcA = (blend >> 8) & 0xf;
    const u32 dstA = (blend >> 12) & 0xf;

    const u32 equRGB = (equation)& 0x7;
    const u32 equA = (equation >> 3) & 0x7;

    drt->SrcBlend = s_blendFactor[srcRGB][0];
    drt->DestBlend = s_blendFactor[dstRGB][0];
    drt->BlendOp = s_blendEquation[equRGB];

    drt->SrcBlendAlpha = s_blendFactor[srcA][1];
    drt->DestBlendAlpha = s_blendFactor[dstA][1];
    drt->BlendOpAlpha = s_blendEquation[equA];

    u8 writeMask = (state&C3_STATE_ALPHA_WRITE)
      ? D3D11_COLOR_WRITE_ENABLE_ALPHA
      : 0
      ;
    writeMask |= (state&C3_STATE_RGB_WRITE)
      ? D3D11_COLOR_WRITE_ENABLE_RED
      | D3D11_COLOR_WRITE_ENABLE_GREEN
      | D3D11_COLOR_WRITE_ENABLE_BLUE
      : 0
      ;

    drt->RenderTargetWriteMask = writeMask;

    if (desc.IndependentBlendEnable) {
      for (u32 ii = 1, rgba_i = rgba; ii < COLOR_ATTACHMENT_COUNT; ++ii, rgba_i >>= 11) {
        drt = &desc.RenderTarget[ii];
        drt->BlendEnable = 0 != (rgba_i & 0x7ff);

        const u32 src = (rgba_i)& 0xf;
        const u32 dst = (rgba_i >> 4) & 0xf;
        const u32 equ = (rgba_i >> 8) & 0x7;

        drt->SrcBlend = s_blendFactor[src][0];
        drt->DestBlend = s_blendFactor[dst][0];
        drt->BlendOp = s_blendEquation[equ];

        drt->SrcBlendAlpha = s_blendFactor[src][1];
        drt->DestBlendAlpha = s_blendFactor[dst][1];
        drt->BlendOpAlpha = s_blendEquation[equ];

        drt->RenderTargetWriteMask = writeMask;
      }
    } else {
      for (u32 ii = 1; ii < COLOR_ATTACHMENT_COUNT; ++ii) {
        memcpy(&desc.RenderTarget[ii], drt, sizeof(D3D11_RENDER_TARGET_BLEND_DESC));
      }
    }

    DX_CHECK(_device->CreateBlendState(&desc, &bs));

    _blend_state_cache.Add(hash, bs);
  }

  _context->OMSetBlendState(bs, blendFactor, 0xffffffff);
}

void GraphicsInterfaceD3D11::SetDepthStencilState(u64 state, u64 stencil) {
  state &= C3_D3D11_DEPTH_STENCIL_MASK;

  u32 fstencil = 0;
  u32 ref = (fstencil & C3_STENCIL_FUNC_REF_MASK) >> C3_STENCIL_FUNC_REF_SHIFT;
  stencil = 0;

  Hasher murmur;
  murmur.Begin();
  murmur.Add(state);
  murmur.Add(stencil);
  u32 hash = murmur.End();

  ID3D11DepthStencilState* dss = _depth_stencil_state_cache.Find(hash);
  if (NULL == dss) {
    D3D11_DEPTH_STENCIL_DESC desc;
    memset(&desc, 0, sizeof(desc));
    u32 func = (state&C3_STATE_DEPTH_TEST_MASK) >> C3_STATE_DEPTH_TEST_SHIFT;
    desc.DepthEnable = 0 != func;
    desc.DepthWriteMask = !!(C3_STATE_DEPTH_WRITE & state) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = s_cmpFunc[func];

    u32 bstencil = 0;
    u32 frontAndBack = bstencil != C3_STENCIL_NONE && bstencil != fstencil;
    bstencil = frontAndBack ? bstencil : fstencil;

    desc.StencilEnable = 0 != stencil;
    desc.StencilReadMask = (fstencil&C3_STENCIL_FUNC_RMASK_MASK) >> C3_STENCIL_FUNC_RMASK_SHIFT;
    desc.StencilWriteMask = 0xff;
    desc.FrontFace.StencilFailOp = s_stencilOp[(fstencil&C3_STENCIL_OP_FAIL_S_MASK) >> C3_STENCIL_OP_FAIL_S_SHIFT];
    desc.FrontFace.StencilDepthFailOp = s_stencilOp[(fstencil&C3_STENCIL_OP_FAIL_Z_MASK) >> C3_STENCIL_OP_FAIL_Z_SHIFT];
    desc.FrontFace.StencilPassOp = s_stencilOp[(fstencil&C3_STENCIL_OP_PASS_Z_MASK) >> C3_STENCIL_OP_PASS_Z_SHIFT];
    desc.FrontFace.StencilFunc = s_cmpFunc[(fstencil&C3_STENCIL_TEST_MASK) >> C3_STENCIL_TEST_SHIFT];
    desc.BackFace.StencilFailOp = s_stencilOp[(bstencil&C3_STENCIL_OP_FAIL_S_MASK) >> C3_STENCIL_OP_FAIL_S_SHIFT];
    desc.BackFace.StencilDepthFailOp = s_stencilOp[(bstencil&C3_STENCIL_OP_FAIL_Z_MASK) >> C3_STENCIL_OP_FAIL_Z_SHIFT];
    desc.BackFace.StencilPassOp = s_stencilOp[(bstencil&C3_STENCIL_OP_PASS_Z_MASK) >> C3_STENCIL_OP_PASS_Z_SHIFT];
    desc.BackFace.StencilFunc = s_cmpFunc[(bstencil&C3_STENCIL_TEST_MASK) >> C3_STENCIL_TEST_SHIFT];

    DX_CHECK(_device->CreateDepthStencilState(&desc, &dss));

    _depth_stencil_state_cache.Add(hash, dss);
  }

  ref = 0;
  _context->OMSetDepthStencilState(dss, ref);
}

void GraphicsInterfaceD3D11::SetDebugWireframe(bool wireframe) {
  if (_wireframe != wireframe) {
    _wireframe = wireframe;
    _rasterizer_state_cache.Invalidate();
  }
}

void GraphicsInterfaceD3D11::SetRasterizerState(u64 state, bool wireframe, bool scissor) {
  state &= C3_STATE_CULL_MASK | C3_STATE_MSAA;
  state |= wireframe ? C3_STATE_PT_LINES : C3_STATE_NONE;
  state |= scissor ? C3_STATE_RESERVED_MASK : 0;

  ID3D11RasterizerState* rs = _rasterizer_state_cache.Find(state);
  if (NULL == rs) {
    u32 cull = (state&C3_STATE_CULL_MASK) >> C3_STATE_CULL_SHIFT;

    D3D11_RASTERIZER_DESC desc;
    desc.FillMode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    desc.CullMode = s_cullMode[cull];
    desc.FrontCounterClockwise = false;
    desc.DepthBias = 0;
    desc.DepthBiasClamp = 0.0f;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthClipEnable = _feature_level <= D3D_FEATURE_LEVEL_9_3;	// disabling depth clip is only supported on 10_0+
    desc.ScissorEnable = scissor;
    //desc.MultisampleEnable = !!(state&C3_STATE_MSAA);
    desc.MultisampleEnable = true;
    desc.AntialiasedLineEnable = false;

    DX_CHECK(_device->CreateRasterizerState(&desc, &rs));

    _rasterizer_state_cache.Add(state, rs);
  }

  _context->RSSetState(rs);
}

void GraphicsInterfaceD3D11::_SetConstantFloat(u8 flags, u32 loc, const void* val, u32 num) {
  _SetConstant(flags, loc, val, num);
}

void GraphicsInterfaceD3D11::_SetConstantVector4(u8 flags, u32 loc, const void* val, u32 num) {
  _SetConstant(flags, loc, val, num * 4);
}

void GraphicsInterfaceD3D11::_SetConstantMatrix4(u8 flags, u32 loc, const void* val, u32 num) {
  _SetConstant(flags, loc, val, num * 16);
}

ID3D11SamplerState* GraphicsInterfaceD3D11::GetSamplerState(u32 flags, const float rgba[4]) {
  const u32 index = (flags & C3_TEXTURE_BORDER_COLOR_MASK) >> C3_TEXTURE_BORDER_COLOR_SHIFT;
  flags &= C3_TEXTURE_SAMPLER_BITS_MASK;

  u32 hash;
  ID3D11SamplerState* sampler;
  if (!need_border_color(flags)) {
    Hasher h;
    h.Begin();
    h.Add(flags);
    h.Add(-1);
    hash = h.End();
    rgba = s_zero.m_zerof;

    sampler = _sampler_state_cache.Find(hash);
  } else {
    Hasher h;
    h.Begin();
    h.Add(flags);
    h.Add(index);
    hash = h.End();
    rgba = NULL == rgba ? s_zero.m_zerof : rgba;

    sampler = _sampler_state_cache.Find(hash);
    if (NULL != sampler) {
      D3D11_SAMPLER_DESC sd;
      sampler->GetDesc(&sd);
      if (0 != memcmp(rgba, sd.BorderColor, 16)) {
        // Sampler will be released when updated sampler
        // is added to cache.
        sampler = NULL;
      }
    }
  }

  if (NULL == sampler) {
    const u32 cmpFunc = (flags&C3_TEXTURE_COMPARE_MASK) >> C3_TEXTURE_COMPARE_SHIFT;
    const u8  minFilter = s_textureFilter[0][(flags&C3_TEXTURE_MIN_MASK) >> C3_TEXTURE_MIN_SHIFT];
    const u8  magFilter = s_textureFilter[1][(flags&C3_TEXTURE_MAG_MASK) >> C3_TEXTURE_MAG_SHIFT];
    const u8  mipFilter = s_textureFilter[2][(flags&C3_TEXTURE_MIP_MASK) >> C3_TEXTURE_MIP_SHIFT];
    const u8  filter = 0 == cmpFunc ? 0 : D3D11_COMPARISON_FILTERING_BIT;

    D3D11_SAMPLER_DESC sd;
    sd.Filter = (D3D11_FILTER)(filter | minFilter | magFilter | mipFilter);
    sd.AddressU = s_textureAddress[(flags&C3_TEXTURE_U_MASK) >> C3_TEXTURE_U_SHIFT];
    sd.AddressV = s_textureAddress[(flags&C3_TEXTURE_V_MASK) >> C3_TEXTURE_V_SHIFT];
    sd.AddressW = s_textureAddress[(flags&C3_TEXTURE_W_MASK) >> C3_TEXTURE_W_SHIFT];
    sd.MipLODBias = 0.0f;
    sd.MaxAnisotropy = _max_anisotropy;
    sd.ComparisonFunc = 0 == cmpFunc ? D3D11_COMPARISON_NEVER : s_cmpFunc[cmpFunc];
    sd.BorderColor[0] = rgba[0];
    sd.BorderColor[1] = rgba[1];
    sd.BorderColor[2] = rgba[2];
    sd.BorderColor[3] = rgba[3];
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    _device->CreateSamplerState(&sd, &sampler);

    _sampler_state_cache.Add(hash, sampler);
  }

  return sampler;
}

void GraphicsInterfaceD3D11::UpdateMsaa() {
  for (u32 ii = 1, last = 0; ii < ARRAY_SIZE(s_msaa); ++ii) {
    u32 msaa = s_checkMsaa[ii];
    u32 quality = 0;
    HRESULT hr = _device->CheckMultisampleQualityLevels(_scd.BufferDesc.Format, msaa, &quality);

    if (SUCCEEDED(hr) && 0 < quality) {
      s_msaa[ii].Count = msaa;
      s_msaa[ii].Quality = quality - 1;
      last = ii;
    } else {
      s_msaa[ii] = s_msaa[last];
    }
  }
}

void GraphicsInterfaceD3D11::UpdateResolution(const Resolution& r) {
  if (r.flags & C3_RESET_MAXANISOTROPY) {
    _max_anisotropy = _max_anisotropy_default;
  } else {
    _max_anisotropy = 1;
  }

  u32 flags = r.flags & ~C3_RESET_MAXANISOTROPY;

  if (_resolution.width != r.width ||
      _resolution.height != r.height ||
      _resolution.flags != flags) {

    bool resize = true && (_resolution.flags & C3_RESET_MSAA_MASK) == (flags & C3_RESET_MSAA_MASK);

    _resolution = r;
    _resolution.flags = flags;

    _scd.BufferDesc.Width = _resolution.width;
    _scd.BufferDesc.Height = _resolution.height;

    PreReset();

    if (resize) {
      _context->OMSetRenderTargets(1, s_zero.m_rtv, NULL);
      DX_CHECK(_swap_chain->ResizeBuffers(2,
        _scd.BufferDesc.Width,
        _scd.BufferDesc.Height,
        _scd.BufferDesc.Format,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    } else {
      UpdateMsaa();
      _scd.SampleDesc = s_msaa[(_resolution.flags & C3_RESET_MSAA_MASK) >> C3_RESET_MSAA_SHIFT];

      DX_RELEASE(_swap_chain);
      HRESULT hr = _factory->CreateSwapChain(_device, &_scd, &_swap_chain);
      if (FAILED(hr)) {
        c3_log("Failed to create swap chain.\n");
        exit(-1);
      }
    }
    
    PostReset();
  }
}

void GraphicsInterfaceD3D11::PreReset() {
  DX_RELEASE(_back_buffer_depth_stencil);
  DX_RELEASE(_back_buffer_color);
  for (u32 ii = 0; ii < ARRAY_SIZE(_frame_buffers); ++ii) _frame_buffers[ii].PreReset();
}

void GraphicsInterfaceD3D11::PostReset() {
  if (_swap_chain) {
    ID3D11Texture2D* color;
    DX_CHECK(_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&color));

    D3D11_RENDER_TARGET_VIEW_DESC desc;
    desc.ViewDimension = (_resolution.flags & C3_RESET_MSAA_MASK) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    desc.Format = (_resolution.flags & C3_RESET_SRGB_BACKBUFFER) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

    DX_CHECK(_device->CreateRenderTargetView(color, &desc, &_back_buffer_color));
    DX_RELEASE(color);
  }
  
  if (!_back_buffer_depth_stencil) {
    D3D11_TEXTURE2D_DESC dsd;
    dsd.Width = _scd.BufferDesc.Width;
    dsd.Height = _scd.BufferDesc.Height;
    dsd.MipLevels = 1;
    dsd.ArraySize = 1;
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsd.SampleDesc = _scd.SampleDesc;
    dsd.Usage = D3D11_USAGE_DEFAULT;
    dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    dsd.CPUAccessFlags = 0;
    dsd.MiscFlags = 0;

    ID3D11Texture2D* depthStencil;
    DX_CHECK(_device->CreateTexture2D(&dsd, NULL, &depthStencil));
    DX_CHECK(_device->CreateDepthStencilView(depthStencil, NULL, &_back_buffer_depth_stencil));
    DX_RELEASE(depthStencil);
  }

  _context->OMSetRenderTargets(1, &_back_buffer_color, _back_buffer_depth_stencil);

  _current_color = _back_buffer_color;
  _current_depth_stencil = _back_buffer_depth_stencil;

  for (u32 ii = 0; ii < ARRAY_SIZE(_frame_buffers); ++ii) _frame_buffers[ii].PostReset();
}

void GraphicsInterfaceD3D11::InvalidateCache() {
  _input_layout_cache.Invalidate();
  _blend_state_cache.Invalidate();
  _depth_stencil_state_cache.Invalidate();
  _rasterizer_state_cache.Invalidate();
  _sampler_state_cache.Invalidate();
  //m_srvUavLru.invalidate();
}

void GraphicsInterfaceD3D11::CommitTextureStage() {
  _context->VSSetShaderResources(0, C3_MAX_TEXTURE_SAMPLERS, _texture_stage._srv);
  _context->VSSetSamplers(0, C3_MAX_TEXTURE_SAMPLERS, _texture_stage._sampler);

  _context->PSSetShaderResources(0, C3_MAX_TEXTURE_SAMPLERS, _texture_stage._srv);
  _context->PSSetSamplers(0, C3_MAX_TEXTURE_SAMPLERS, _texture_stage._sampler);
}

void GraphicsInterfaceD3D11::InvalidateTextureStage() {
  _texture_stage.Clear();
  CommitTextureStage();
}
