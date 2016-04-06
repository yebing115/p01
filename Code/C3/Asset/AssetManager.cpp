#include "C3PCH.h"
#include "AssetManager.h"
#include "DDSTextureLoader.h"
#include "MEXModelLoader.h"

DEFINE_SINGLETON_INSTANCE(AssetManager);

static AssetOperations NULL_ASSET_OPS = {
  [](Asset*) -> atomic_int* { return nullptr; },
  [](Asset*) {},
};

struct AssetLoaderType {
  AssetType _type;
  const char* _suffix;
  AssetOperations* _ops;
};

static AssetLoaderType ASSET_LOADERS[] = {
  {ASSET_TYPE_TEXTURE, ".dds", &DDS_TEXTURE_OPS},
  {ASSET_TYPE_MODEL, ".mex", &MEX_MODEL_OPS},
};

AssetManager::AssetManager(): _num_assets(0) {
  _asset_map.reserve(C3_MAX_ASSETS);
}

AssetManager::~AssetManager() {}

Asset* AssetManager::Get(const AssetDesc& desc) {
  auto it = _asset_map.find(String::GetID(desc._filename));
  return (it == _asset_map.end() ? nullptr : _assets + it->second);
}

void AssetManager::Load(Asset* asset) {
  JobScheduler::Instance()->WaitJobs(LoadAsync(asset));
}

atomic_int* AssetManager::LoadAsync(Asset* asset) {
  c3_assert_return_x(asset, nullptr);
  AssetState old_state;
retry:
  old_state = asset->_state;
  while (old_state == ASSET_STATE_LOADING || old_state == ASSET_STATE_UNLOADING) {
    JobScheduler::Instance()->Yield();
    old_state = asset->_state;
  }
  if (old_state == ASSET_STATE_READY) return nullptr;
  while (!asset->_state.compare_exchange_strong(old_state, ASSET_STATE_LOADING))
    goto retry;
  return asset->_desc._ops->_load_async_fn(asset);
}

// TODO: check asset->_state
void AssetManager::Unload(Asset* asset) {
  c3_assert_return(asset);
  if (asset->_ref == 0) return;
  --asset->_ref;
  if (asset->_ref == 0 && asset->_state == ASSET_STATE_READY) {
    asset->_state = ASSET_STATE_UNLOADING;
    asset->_desc._ops->_unload_fn(asset);
  }
}

AssetDesc AssetManager::Resolve(AssetType type, const char* filename) {
  AssetDesc desc;
  desc._type = type;
  desc._flags = 0;
  desc._ops = &NULL_ASSET_OPS;
  strcpy(desc._filename, filename);

  const char* suffix = strrchr(filename, '.');
  if (!suffix) return desc;
  for (AssetLoaderType* loader = ASSET_LOADERS; loader < ASSET_LOADERS + ARRAY_SIZE(ASSET_LOADERS); ++loader) {
    if (loader->_type == type && strcmp(loader->_suffix, suffix) == 0) {
      desc._ops = loader->_ops;
      break;
    }
  }

  return desc;
}

Asset* AssetManager::GetOrCreateAsset(const AssetDesc& desc) {
  auto asset_id = String::GetID(desc._filename);
  auto it = _asset_map.find(asset_id);
  if (it != _asset_map.end()) {
    auto asset = _assets + it->second;
    ++asset->_ref;
    return asset;
  }
  if (_num_assets >= C3_MAX_ASSETS) {
    c3_log("Asset usage reach limit C3_MAX_ASSETS = %s.\n", C3_MAX_ASSETS);
    return nullptr;
  }
  Asset* asset = _assets + _num_assets;
  _asset_map[asset_id] = _num_assets;
  ++_num_assets;
  asset->_state = ASSET_STATE_EMPTY;
  asset->_desc = desc;
  asset->_header = nullptr;
  asset->_ref = 1;
  return asset;
}
