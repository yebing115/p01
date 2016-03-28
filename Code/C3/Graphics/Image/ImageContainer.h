#pragma once
#include "Data/DataType.h"

struct ImageContainer {
	void* data;
	u32 size;
	u32 offset;
	u16 width;
	u16 height;
	u16 depth;
	u8 format;
	u8 num_mips;
	bool has_alpha;
	bool cube_map;
	bool srgb;
};

struct ImageMip {
	u16 width;
	u16 height;
	u16 block_size;
	u32 size;
	u8 bpp;
	u8 format;
	bool has_alpha;
	const void* data;
};

enum ImageEncodingType {
	IMAGE_ENCODING_UNORM,
	IMAGE_ENCODING_INT,
	IMAGE_ENCODING_u,
	IMAGE_ENCODING_FLOAT,
	IMAGE_ENCODING_SNORM,
	IMAGE_ENCODING_COUNT,
};

struct ImageBlockInfo {
	u8 bits_per_pixel;
	u8 block_width;
	u8 block_height;
	u8 block_size;
	u8 min_block_x;
	u8 min_block_y;
	u8 depth_bits;
	u8 stencil_bits;
	u8 encoding;
};
