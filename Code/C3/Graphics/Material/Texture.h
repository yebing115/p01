#pragma once
#include "Pattern/Handle.h"
#include "Graphics/GraphicsInterface.h"

struct Texture {
  Handle _handle;
  TextureInfo _info;
  const MemoryBlock* _mem;
};
