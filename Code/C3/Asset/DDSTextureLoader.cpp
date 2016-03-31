#include "C3PCH.h"
#include "DDSTextureLoader.h"

static void dds_load(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_LOADING);
}

static void dds_unload(Asset* asset) {
  c3_assert(asset->_state == ASSET_STATE_UNLOADING);
}

AssetOperations DDS_TEXTURE_OPS = {
  &dds_load,
  &dds_unload,
};
