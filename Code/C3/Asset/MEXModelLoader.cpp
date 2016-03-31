#include "C3PCH.h"
#include "MEXModelLoader.h"

static void mex_load(Asset* asset) {}

static void mex_unload(Asset* asset) {}

AssetOperations MEX_TEXTURE_OPS = {
  &mex_load,
  &mex_unload,
};

