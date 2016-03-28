#pragma once
#include "dx_header.h"
#include "Graphics/GraphicsTypes.h"
#include "Data/DataType.h"
#include "Platform/PlatformConfig.h"

struct PrimInfo {
	D3D11_PRIMITIVE_TOPOLOGY type;
	u32 min;
	u32 div;
	u32 sub;
};

extern const PrimInfo PRIM_INFO[6];
