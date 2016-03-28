#include "ImageUtils.h"
#include "Data/Blob.h"
#include "Graphics/GraphicsRenderer.h"

// DDS
#define DDS_MAGIC             MAKE_FOURCC('D', 'D', 'S', ' ')
#define DDS_HEADER_SIZE       124

#define DDS_DXT1 MAKE_FOURCC('D', 'X', 'T', '1')
#define DDS_DXT2 MAKE_FOURCC('D', 'X', 'T', '2')
#define DDS_DXT3 MAKE_FOURCC('D', 'X', 'T', '3')
#define DDS_DXT4 MAKE_FOURCC('D', 'X', 'T', '4')
#define DDS_DXT5 MAKE_FOURCC('D', 'X', 'T', '5')
#define DDS_ATI1 MAKE_FOURCC('A', 'T', 'I', '1')
#define DDS_BC4U MAKE_FOURCC('B', 'C', '4', 'U')
#define DDS_ATI2 MAKE_FOURCC('A', 'T', 'I', '2')
#define DDS_BC5U MAKE_FOURCC('B', 'C', '5', 'U')
#define DDS_DX10 MAKE_FOURCC('D', 'X', '1', '0')

#define DDS_A8R8G8B8       21
#define DDS_R5G6B5         23
#define DDS_A1R5G5B5       25
#define DDS_A4R4G4B4       26
#define DDS_A2B10G10R10    31
#define DDS_G16R16         34
#define DDS_A2R10G10B10    35
#define DDS_A16B16G16R16   36
#define DDS_A8L8           51
#define DDS_R16F           111
#define DDS_G16R16F        112
#define DDS_A16B16G16R16F  113
#define DDS_R32F           114
#define DDS_G32R32F        115
#define DDS_A32B32G32R32F  116

#define DDS_FORMAT_R32G32B32A32_FLOAT  2
#define DDS_FORMAT_R32G32B32A32_UINT   3
#define DDS_FORMAT_R16G16B16A16_FLOAT  10
#define DDS_FORMAT_R16G16B16A16_UNORM  11
#define DDS_FORMAT_R16G16B16A16_UINT   12
#define DDS_FORMAT_R32G32_FLOAT        16
#define DDS_FORMAT_R32G32_UINT         17
#define DDS_FORMAT_R10G10B10A2_UNORM   24
#define DDS_FORMAT_R11G11B10_FLOAT     26
#define DDS_FORMAT_R8G8B8A8_UNORM      28
#define DDS_FORMAT_R8G8B8A8_UNORM_SRGB 29
#define DDS_FORMAT_R16G16_FLOAT        34
#define DDS_FORMAT_R16G16_UNORM        35
#define DDS_FORMAT_R32_FLOAT           41
#define DDS_FORMAT_R32_UINT            42
#define DDS_FORMAT_R8G8_UNORM          49
#define DDS_FORMAT_R16_FLOAT           54
#define DDS_FORMAT_R16_UNORM           56
#define DDS_FORMAT_R8_UNORM            61
#define DDS_FORMAT_R1_UNORM            66
#define DDS_FORMAT_BC1_UNORM           71
#define DDS_FORMAT_BC1_UNORM_SRGB      72
#define DDS_FORMAT_BC2_UNORM           74
#define DDS_FORMAT_BC2_UNORM_SRGB      75
#define DDS_FORMAT_BC3_UNORM           77
#define DDS_FORMAT_BC3_UNORM_SRGB      78
#define DDS_FORMAT_BC4_UNORM           80
#define DDS_FORMAT_BC5_UNORM           83
#define DDS_FORMAT_B5G6R5_UNORM        85
#define DDS_FORMAT_B5G5R5A1_UNORM      86
#define DDS_FORMAT_B8G8R8A8_UNORM      87
#define DDS_FORMAT_B8G8R8A8_UNORM_SRGB 91
#define DDS_FORMAT_BC6H_SF16           96
#define DDS_FORMAT_BC7_UNORM           98
#define DDS_FORMAT_BC7_UNORM_SRGB      99
#define DDS_FORMAT_B4G4R4A4_UNORM      115

#define DDSD_CAPS                   0x00000001
#define DDSD_HEIGHT                 0x00000002
#define DDSD_WIDTH                  0x00000004
#define DDSD_PITCH                  0x00000008
#define DDSD_PIXELFORMAT            0x00001000
#define DDSD_MIPMAPCOUNT            0x00020000
#define DDSD_LINEARSIZE             0x00080000
#define DDSD_DEPTH                  0x00800000

#define DDPF_ALPHAPIXELS            0x00000001
#define DDPF_ALPHA                  0x00000002
#define DDPF_FOURCC                 0x00000004
#define DDPF_INDEXED                0x00000020
#define DDPF_RGB                    0x00000040
#define DDPF_YUV                    0x00000200
#define DDPF_LUMINANCE              0x00020000

#define DDSCAPS_COMPLEX             0x00000008
#define DDSCAPS_TEXTURE             0x00001000
#define DDSCAPS_MIPMAP              0x00400000

#define DDSCAPS2_cube_map            0x00000200
#define DDSCAPS2_cube_map_POSITIVEX  0x00000400
#define DDSCAPS2_cube_map_NEGATIVEX  0x00000800
#define DDSCAPS2_cube_map_POSITIVEY  0x00001000
#define DDSCAPS2_cube_map_NEGATIVEY  0x00002000
#define DDSCAPS2_cube_map_POSITIVEZ  0x00004000
#define DDSCAPS2_cube_map_NEGATIVEZ  0x00008000

#define DDS_cube_map_ALLFACES (DDSCAPS2_cube_map_POSITIVEX|DDSCAPS2_cube_map_NEGATIVEX \
							 |DDSCAPS2_cube_map_POSITIVEY|DDSCAPS2_cube_map_NEGATIVEY \
							 |DDSCAPS2_cube_map_POSITIVEZ|DDSCAPS2_cube_map_NEGATIVEZ)

#define DDSCAPS2_VOLUME             0x00200000

struct TranslateDDSFormat {
  u32 format;
  TextureFormat texture_format;
  bool srgb;
};

static TranslateDDSFormat s_translate_dds_fourcc_format[] =
{
  {DDS_DXT1, DXT1_RGB_TEXTURE_FORMAT, false},
  {DDS_DXT2, DXT3_ARGB_TEXTURE_FORMAT, false},
  {DDS_DXT3, DXT3_ARGB_TEXTURE_FORMAT, false},
  {DDS_DXT4, DXT5_ARGB_TEXTURE_FORMAT, false},
  {DDS_DXT5, DXT5_ARGB_TEXTURE_FORMAT, false},
};

static TranslateDDSFormat s_translate_dxgi_format[] =
{
  {DDS_FORMAT_BC1_UNORM, DXT1_RGB_TEXTURE_FORMAT, false},
  {DDS_FORMAT_BC1_UNORM_SRGB, DXT1_ARGB_TEXTURE_FORMAT, true},
  {DDS_FORMAT_BC2_UNORM, DXT3_ARGB_TEXTURE_FORMAT, false},
  {DDS_FORMAT_BC2_UNORM_SRGB, DXT3_ARGB_TEXTURE_FORMAT, true},
  {DDS_FORMAT_BC3_UNORM, DXT5_ARGB_TEXTURE_FORMAT, false},
  {DDS_FORMAT_BC3_UNORM_SRGB, DXT5_ARGB_TEXTURE_FORMAT, true},
  {DDS_FORMAT_B8G8R8A8_UNORM, RGBA_8_TEXTURE_FORMAT, false},
  {DDS_FORMAT_R8G8B8A8_UNORM_SRGB, RGBA_8_TEXTURE_FORMAT, true},
  {DDS_FORMAT_R8_UNORM, RED_8_TEXTURE_FORMAT, false},
};

struct TranslateDDSPixelFormat {
  u32 bit_count;
  u32 bitmask[4];
  TextureFormat texture_format;
};

static TranslateDDSPixelFormat s_translate_dds_pixel_format[] =
{
  {8, {0x000000ff, 0x00000000, 0x00000000, 0x00000000}, RED_8_TEXTURE_FORMAT},
  {32, {0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000}, RGBA_8_TEXTURE_FORMAT},
  {32, {0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000}, RGBA_8_TEXTURE_FORMAT},
};

static const ImageBlockInfo s_image_block_info[] = {
// +------------------------------- bits per pixel
// |  +---------------------------- block width
// |  |  +------------------------- block height
// |  |  |  +--------------------- block size
// |  |  |  |  +------------------ min blocks x
// |  |  |  |  |  +--------------- min blocks y
// |  |  |  |  |  |  +----------- depth bits
// |  |  |  |  |  |  |  +-------- stencil bits
// |  |  |  |  |  |  |  |  +----- encoding type
// |  |  |  |  |  |  |  |  |
  {8, 1, 1, 1, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // RED_8_TEXTURE_FORMAT
  {16, 1, 1, 2, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // RG_8_TEXTURE_FORMAT
  {24, 1, 1, 3, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // RGB_8_TEXTURE_FORMAT
  {32, 1, 1, 4, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // RGBA_8_TEXTURE_FORMAT
  {32, 1, 1, 1, 1, 1, 0, 0, u8(IMAGE_ENCODING_FLOAT)}, // RED_32_FLOAT_TEXTURE_FORMAT
  {48, 1, 1, 3, 1, 1, 0, 0, u8(IMAGE_ENCODING_FLOAT)}, // RGB_16_FLOAT_TEXTURE_FORMAT
  {64, 1, 1, 4, 1, 1, 0, 0, u8(IMAGE_ENCODING_FLOAT)}, // RGBA_16_FLOAT_TEXTURE_FORMAT
  {96, 1, 1, 3, 1, 1, 0, 0, u8(IMAGE_ENCODING_FLOAT)}, // RGB_32_FLOAT_TEXTURE_FORMAT
  {128, 1, 1, 4, 1, 1, 0, 0, u8(IMAGE_ENCODING_FLOAT)}, // RGBA_32_FLOAT_TEXTURE_FORMAT
  {16, 1, 1, 2, 1, 1, 16, 0, u8(IMAGE_ENCODING_UNORM)}, // DEPTH_16_TEXTURE_FORMAT
  {24, 1, 1, 3, 1, 1, 24, 0, u8(IMAGE_ENCODING_UNORM)}, // DEPTH_24_TEXTURE_FORMAT
  {32, 1, 1, 4, 1, 1, 32, 0, u8(IMAGE_ENCODING_UNORM)}, // DEPTH_32_FLOAT_TEXTURE_FORMAT
  {4, 4, 4, 8, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // DXT1_RGB_TEXTURE_FORMAT
  {4, 4, 4, 8, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // DXT1_ARGB_TEXTURE_FORMAT
  {8, 4, 4, 16, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // DXT3_ARGB_TEXTURE_FORMAT
  {8, 4, 4, 16, 1, 1, 0, 0, u8(IMAGE_ENCODING_UNORM)}, // DXT5_ARGB_TEXTURE_FORMAT
};

bool image_parse_dds(ImageContainer& image_container_out, BlobReader& stream) {
  u32 header_size;
  stream.Read(header_size);

  if (header_size < DDS_HEADER_SIZE) return false;

  u32 flags;
  stream.Read(flags);

  if ((flags & (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT)) != (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT)) {
    return false;
  }

  u32 height;
  stream.Read(height);

  u32 width;
  stream.Read(width);

  u32 pitch;
  stream.Read(pitch);

  u32 depth;
  stream.Read(depth);

  u32 mips;
  stream.Read(mips);

  stream.Skip(44); // reserved

  u32 pixel_format_size;
  stream.Read(pixel_format_size);

  u32 pixel_flags;
  stream.Read(pixel_flags);

  u32 fourcc;
  stream.Read(fourcc);

  u32 bit_count;
  stream.Read(bit_count);

  u32 bitmask[4];
  stream.Read((void*)bitmask, sizeof(bitmask));

  u32 caps[4];
  stream.Read(caps);

  stream.Skip(4); // reserved

  u32 dxgi_format = 0;
  if (DDPF_FOURCC == pixel_flags &&  DDS_DX10 == fourcc) {
    stream.Read(dxgi_format);

    u32 dims;
    stream.Read(dims);

    u32 misc_flags;
    stream.Read(misc_flags);

    u32 array_size;
    stream.Read(array_size);

    u32 misc_flags2;
    stream.Read(misc_flags2);
  }

  if ((caps[0] & DDSCAPS_TEXTURE) == 0) return false;

  bool cube_map = 0 != (caps[1] & DDSCAPS2_cube_map);
  if (cube_map) {
    if ((caps[1] & DDS_cube_map_ALLFACES) != DDS_cube_map_ALLFACES) {
      // partial cube map is not supported.
      return false;
    }
  }

  TextureFormat format = INVALID_TEXTURE_FORMAT;
  bool has_alpha = pixel_flags & DDPF_ALPHAPIXELS;
  bool srgb = false;

  if (dxgi_format == 0) {
    if (DDPF_FOURCC == (pixel_flags & DDPF_FOURCC)) {
      for (u32 ii = 0; ii < ARRAY_SIZE(s_translate_dds_fourcc_format); ++ii) {
        if (s_translate_dds_fourcc_format[ii].format == fourcc) {
          format = s_translate_dds_fourcc_format[ii].texture_format;
          break;
        }
      }
    } else {
      for (u32 ii = 0; ii < ARRAY_SIZE(s_translate_dds_pixel_format); ++ii) {
        const TranslateDDSPixelFormat& pf = s_translate_dds_pixel_format[ii];
        if (pf.bit_count == bit_count
            &&  pf.bitmask[0] == bitmask[0]
            && pf.bitmask[1] == bitmask[1]
            && pf.bitmask[2] == bitmask[2]
            && pf.bitmask[3] == bitmask[3]) {
          format = pf.texture_format;
          break;
        }
      }
    }
  } else {
    for (u32 ii = 0; ii < ARRAY_SIZE(s_translate_dxgi_format); ++ii) {
      if (s_translate_dxgi_format[ii].format == dxgi_format) {
        format = s_translate_dxgi_format[ii].texture_format;
        srgb = s_translate_dxgi_format[ii].srgb;
        break;
      }
    }
  }

  image_container_out.data = NULL;
  image_container_out.size = 0;
  image_container_out.offset = stream.GetPosition();
  image_container_out.width = u16(width);
  image_container_out.height = u16(height);
  image_container_out.depth = u16(depth);
  image_container_out.format = u8(format);
  image_container_out.num_mips = u8((caps[0] & DDSCAPS_MIPMAP) ? mips : 1);
  image_container_out.has_alpha = has_alpha;
  image_container_out.cube_map = cube_map;
  image_container_out.srgb = srgb;

  return format != INVALID_TEXTURE_FORMAT;
}

bool image_parse(ImageContainer& image_container_out, const void* data, u32 size) {
  BlobReader stream(data, size);
  u32 magic;
  stream.Read(magic);

  if (magic == DDS_MAGIC) {
    return image_parse_dds(image_container_out, stream);
  } else if (magic == C3_CHUNK_MAGIC_TEX) {
    TextureCreate tc;
    stream.Read(tc);

    image_container_out.format = tc.format;
    image_container_out.offset = UINT32_MAX;
    if (!tc.mem) {
      image_container_out.data = nullptr;
      image_container_out.size = 0;
    } else {
      image_container_out.data = tc.mem->data;
      image_container_out.size = tc.mem->size;
    }
    image_container_out.width = tc.width;
    image_container_out.height = tc.height;
    image_container_out.depth = tc.depth;
    image_container_out.num_mips = tc.num_mips;
    image_container_out.has_alpha = false;
    image_container_out.cube_map = tc.cube_map;
    image_container_out.srgb = false;

    return true;
  }

  return false;
}

bool image_get_raw_data(const ImageContainer& image_container, u8 side_, u8 lod_, const void* data, u32 data_size, ImageMip& mip_out) {
  u32 offset = image_container.offset;
  TextureFormat format = (TextureFormat)image_container.format;
  bool has_alpha = image_container.has_alpha;

  const ImageBlockInfo& ibi = s_image_block_info[format];
  const u8  bpp = ibi.bits_per_pixel;
  const u16 block_size = ibi.block_size;
  const u16 block_width = ibi.block_width;
  const u16 block_height = ibi.block_height;
  const u16 min_block_x = ibi.min_block_x;
  const u16 min_block_y = ibi.min_block_y;

  if (image_container.offset == UINT32_MAX) {
    if (!image_container.data) return false;

    offset = 0;
    data = image_container.data;
    data_size = image_container.size;
  }
  for (u8 side = 0, numSides = image_container.cube_map ? 6 : 1; side < numSides; ++side) {
    u16 width = image_container.width;
    u16 height = image_container.height;
    u16 depth = max<u16>(1, image_container.depth);

    for (u8 lod = 0, num = image_container.num_mips; lod < num; ++lod) {
      u32 lod_width = max(block_width * min_block_x, (width + block_width - 1) / block_width * block_width);
      u32 lod_height = max(block_height * min_block_y, (height + block_height - 1) / block_height * block_height);
      u32 size = lod_width * lod_height * depth * bpp / 8;

      if (side == side_ &&  lod == lod_) {
        mip_out.width = width;
        mip_out.height = height;
        mip_out.block_size = block_size;
        mip_out.size = size;
        mip_out.data = (const u8*)data + offset;
        mip_out.bpp = bpp;
        mip_out.format = u8(format);
        mip_out.has_alpha = has_alpha;
        return true;
      }

      offset += size;

      if (offset > data_size) {
        c3_log("Reading past size of data buffer, offset = %u, size = %u!", offset, data_size);
        return false;
      }

      width = max<u16>(1, width / 2);
      height = max<u16>(1, height / 2);
      depth = max<u16>(1, depth / 2);
    }
  }

  return false;
}

void image_get_rgba8_data(void* dst, const void* src, u16 width, u16 height, u32 pitch, u8 type) {
  switch (type) {
  case RGBA_8_TEXTURE_FORMAT:
    memcpy(dst, src, pitch * height);
    break;

  default:
    image_get_bgra8_data(dst, src, width, height, pitch, type);
    image_swizzle_bgra8(width, height, pitch, dst, dst);
    break;
  }
}

static u8 bit_range_convert(u32 v, u32 from, u32 to) {
  u32 tmp0 = 1 << to;
  u32 tmp1 = 1 << from;
  u32 tmp2 = tmp0 - 1;
  u32 tmp3 = tmp1 - 1;
  u32 tmp4 = v * tmp2;
  u32 tmp5 = tmp3 + tmp4;
  u32 tmp6 = tmp5 >> from;
  u32 tmp7 = tmp5 + tmp6;
  u32 result = tmp7 >> from;

  return u8(result);
}

static void decode_block_dxt(u8 dst[16 * 4], const u8 src[8]) {
  u8 colors[4 * 3];

  u32 c0 = src[0] | (src[1] << 8);
  colors[0] = bit_range_convert((c0 >> 0) & 0x1f, 5, 8);
  colors[1] = bit_range_convert((c0 >> 5) & 0x3f, 6, 8);
  colors[2] = bit_range_convert((c0 >> 11) & 0x1f, 5, 8);

  u32 c1 = src[2] | (src[3] << 8);
  colors[3] = bit_range_convert((c1 >> 0) & 0x1f, 5, 8);
  colors[4] = bit_range_convert((c1 >> 5) & 0x3f, 6, 8);
  colors[5] = bit_range_convert((c1 >> 11) & 0x1f, 5, 8);

  colors[6] = (2 * colors[0] + colors[3]) / 3;
  colors[7] = (2 * colors[1] + colors[4]) / 3;
  colors[8] = (2 * colors[2] + colors[5]) / 3;

  colors[9] = (colors[0] + 2 * colors[3]) / 3;
  colors[10] = (colors[1] + 2 * colors[4]) / 3;
  colors[11] = (colors[2] + 2 * colors[5]) / 3;

  for (u32 ii = 0, next = 8 * 4; ii < 16 * 4; ii += 4, next += 2) {
    int idx = ((src[next >> 3] >> (next & 7)) & 3) * 3;
    dst[ii + 0] = colors[idx + 0];
    dst[ii + 1] = colors[idx + 1];
    dst[ii + 2] = colors[idx + 2];
  }
}

static void decode_block_dxt1(u8 dst[16 * 4], const u8 src[8]) {
  u8 colors[4 * 4];

  u32 c0 = src[0] | (src[1] << 8);
  colors[0] = bit_range_convert((c0 >> 0) & 0x1f, 5, 8);
  colors[1] = bit_range_convert((c0 >> 5) & 0x3f, 6, 8);
  colors[2] = bit_range_convert((c0 >> 11) & 0x1f, 5, 8);
  colors[3] = 255;

  u32 c1 = src[2] | (src[3] << 8);
  colors[4] = bit_range_convert((c1 >> 0) & 0x1f, 5, 8);
  colors[5] = bit_range_convert((c1 >> 5) & 0x3f, 6, 8);
  colors[6] = bit_range_convert((c1 >> 11) & 0x1f, 5, 8);
  colors[7] = 255;

  if (c0 > c1) {
    colors[8] = (2 * colors[0] + colors[4]) / 3;
    colors[9] = (2 * colors[1] + colors[5]) / 3;
    colors[10] = (2 * colors[2] + colors[6]) / 3;
    colors[11] = 255;

    colors[12] = (colors[0] + 2 * colors[4]) / 3;
    colors[13] = (colors[1] + 2 * colors[5]) / 3;
    colors[14] = (colors[2] + 2 * colors[6]) / 3;
    colors[15] = 255;
  } else {
    colors[8] = (colors[0] + colors[4]) / 2;
    colors[9] = (colors[1] + colors[5]) / 2;
    colors[10] = (colors[2] + colors[6]) / 2;
    colors[11] = 255;

    colors[12] = 0;
    colors[13] = 0;
    colors[14] = 0;
    colors[15] = 0;
  }

  for (u32 ii = 0, next = 8 * 4; ii < 16 * 4; ii += 4, next += 2) {
    int idx = ((src[next >> 3] >> (next & 7)) & 3) * 4;
    dst[ii + 0] = colors[idx + 0];
    dst[ii + 1] = colors[idx + 1];
    dst[ii + 2] = colors[idx + 2];
    dst[ii + 3] = colors[idx + 3];
  }
}

static void decode_block_dxt23a(u8 dst[16 * 4], const u8 src[8]) {
  for (u32 ii = 0, next = 0; ii < 16 * 4; ii += 4, next += 4) {
    u32 c0 = (src[next >> 3] >> (next & 7)) & 0xf;
    dst[ii] = bit_range_convert(c0, 4, 8);
  }
}

static void decode_block_dxt45a(u8 dst[16 * 4], const u8 src[8]) {
  u8 alpha[8];
  alpha[0] = src[0];
  alpha[1] = src[1];

  if (alpha[0] > alpha[1]) {
    alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;
    alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
    alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
    alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
    alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
    alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;
  } else {
    alpha[2] = (4 * alpha[0] + 1 * alpha[1]) / 5;
    alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
    alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
    alpha[5] = (1 * alpha[0] + 4 * alpha[1]) / 5;
    alpha[6] = 0;
    alpha[7] = 255;
  }

  u32 idx0 = src[2];
  u32 idx1 = src[5];
  idx0 |= u32(src[3]) << 8;
  idx1 |= u32(src[6]) << 8;
  idx0 |= u32(src[4]) << 16;
  idx1 |= u32(src[7]) << 16;
  for (u32 ii = 0; ii < 8 * 4; ii += 4) {
    dst[ii] = alpha[idx0 & 7];
    dst[ii + 32] = alpha[idx1 & 7];
    idx0 >>= 3;
    idx1 >>= 3;
  }
}

void image_get_bgra8_data(void* dst_, const void* src_, u16 width_, u16 height_, u32 pitch, u8 type) {
  u16 width = width_ / 4;
  u16 height = height_ / 4;

  u8 temp[16 * 4];
  const u8* src = (const u8*)src_;

  switch (type) {
  case DXT1_RGB_TEXTURE_FORMAT:
  case DXT1_ARGB_TEXTURE_FORMAT:
    for (u16 yy = 0; yy < height; ++yy) {
      for (u16 xx = 0; xx < width; ++xx) {
        decode_block_dxt1(temp, src);
        src += 8;

        u8* dst = (u8*)dst_ + (yy * pitch + xx * 4) * 4;
        memcpy(&dst[0 * pitch], &temp[0], 16);
        memcpy(&dst[1 * pitch], &temp[16], 16);
        memcpy(&dst[2 * pitch], &temp[32], 16);
        memcpy(&dst[3 * pitch], &temp[48], 16);
      }
    }
    break;

  case DXT3_ARGB_TEXTURE_FORMAT:
    for (u16 yy = 0; yy < height; ++yy) {
      for (u16 xx = 0; xx < width; ++xx) {
        decode_block_dxt23a(temp + 3, src);
        src += 8;
        decode_block_dxt(temp, src);
        src += 8;

        u8* dst = (u8*)dst_ + (yy * pitch + xx * 4) * 4;
        memcpy(&dst[0 * pitch], &temp[0], 16);
        memcpy(&dst[1 * pitch], &temp[16], 16);
        memcpy(&dst[2 * pitch], &temp[32], 16);
        memcpy(&dst[3 * pitch], &temp[48], 16);
      }
    }
    break;

  case DXT5_ARGB_TEXTURE_FORMAT:
    for (u16 yy = 0; yy < height; ++yy) {
      for (u16 xx = 0; xx < width; ++xx) {
        decode_block_dxt45a(temp + 3, src);
        src += 8;
        decode_block_dxt(temp, src);
        src += 8;

        u8* dst = (u8*)dst_ + (yy * pitch + xx * 4) * 4;
        memcpy(&dst[0 * pitch], &temp[0], 16);
        memcpy(&dst[1 * pitch], &temp[16], 16);
        memcpy(&dst[2 * pitch], &temp[32], 16);
        memcpy(&dst[3 * pitch], &temp[48], 16);
      }
    }
    break;


  default:
    c3_log("Not supported image bgra8 decode.\n");
    // Decompression not implemented... Make ugly red-yellow checkerboard texture.
    image_checkerboard(width_, height_, 16, UINT32_C(0xffff0000), UINT32_C(0xffffff00), dst_);
    break;
  }
}

void image_swizzle_bgra8(u16 width, u16 height, u32 src_pitch, const void* src_, void* dst_) {
  const u8* src = (const u8*)src_;
  u8* dst = (u8*)dst_;
  const u8* next = src + src_pitch;

  for (u16 yy = 0; yy < height; ++yy, src = next, next += src_pitch) {
    for (u16 xx = 0; xx < width; ++xx, src += 4, dst += 4) {
      u8 rr = src[0];
      u8 gg = src[1];
      u8 bb = src[2];
      u8 aa = src[3];
      dst[0] = bb;
      dst[1] = gg;
      dst[2] = rr;
      dst[3] = aa;
    }
  }
}

void image_swizzle_bgr8(u16 width, u16 height, u32 src_pitch, const void* src_, void* dst_) {
  const u8* src = (const u8*)src_;
  u8* dst = (u8*)dst_;
  const u8* next = src + src_pitch;

  for (u16 yy = 0; yy < height; ++yy, src = next, next += src_pitch) {
    for (u16 xx = 0; xx < width; ++xx, src += 3, dst += 3) {
      u8 rr = src[0];
      u8 gg = src[1];
      u8 bb = src[2];
      dst[0] = bb;
      dst[1] = gg;
      dst[2] = rr;
    }
  }
}

void image_checkerboard(u16 width, u16 height, u16 step, u32 _0, u32 _1, void* dst_) {
  u32* dst = (u32*)dst_;
  for (u16 yy = 0; yy < height; ++yy) {
    for (u16 xx = 0; xx < width; ++xx) {
      u32 abgr = ((xx / step) & 1) ^ ((yy / step) & 1) ? _1 : _0;
      *dst++ = abgr;
    }
  }
}

const ImageBlockInfo& image_block_info(TextureFormat format) {
  return s_image_block_info[format];
}
