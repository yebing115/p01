#pragma once
#include "ImageContainer.h"
#include "Graphics/GraphicsTypes.h"

bool image_parse(ImageContainer& image_container_out, const void* data, u32 size);
bool image_get_raw_data(const ImageContainer& image_container, u8 side, u8 lod, const void* data, u32 size, ImageMip& mip_out);
void image_get_rgba8_data(void* dst, const void* src, u16 width, u16 height, u32 pitch, u8 type);
void image_get_bgra8_data(void* dst, const void* src, u16 width, u16 height, u32 pitch, u8 type);
void image_checkerboard(u16 width, u16 height, u16 step, u32 _0, u32 _1, void* dst);
void image_swizzle_bgra8(u16 width, u16 height, u32 src_pitch, const void* src, void* dst);
void image_swizzle_bgr8(u16 width, u16 height, u32 src_pitch, const void* src, void* dst);
const ImageBlockInfo& image_block_info(TextureFormat format);
inline u8 image_bits_per_pixel(TextureFormat format) { return image_block_info(format).bits_per_pixel; }
inline u8 image_block_size(TextureFormat format) { return image_block_info(format).block_size; }
