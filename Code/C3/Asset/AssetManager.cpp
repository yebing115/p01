#include "C3PCH.h"
#include "AssetManager.h"
#include "DDSTextureLoader.h"
#include "MEXModelLoader.h"
#include "MaterialLoader.h"

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
  {ASSET_TYPE_MATERIAL_SHADER, ".mas", &MATERIAL_SHADER_OPS},
  {ASSET_TYPE_MATERIAL, ".mat", &MATERIAL_OPS},
};

AssetManager::AssetManager(): _num_assets(0) {
  _asset_map.reserve(C3_MAX_ASSETS);
}

AssetManager::~AssetManager() {}

Asset* AssetManager::Get(AssetType type, const char* filename) {
  AssetDesc desc;
  AssetOperations* ops;
  bool resolved = Resolve(type, filename, desc, ops);
  if (!resolved) return nullptr;
  return GetOrCreateAsset(desc, ops);
}

void AssetManager::Load(Asset* asset) {
  JobScheduler::Instance()->WaitJobs(LoadAsync(asset));
}

Asset* AssetManager::Load(AssetType type, const char* filename) {
  AssetDesc desc;
  AssetOperations* ops;
  bool resolved = Resolve(type, filename, desc, ops);
  if (!resolved) return nullptr;
  Asset* asset = GetOrCreateAsset(desc, ops);
  Load(asset);
  return asset;
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
  return asset->_ops->_load_async_fn(asset);
}

atomic_int* AssetManager::LoadAsync(AssetType type, const char* filename) {
  AssetDesc desc;
  AssetOperations* ops;
  bool resolved = Resolve(type, filename, desc, ops);
  if (!resolved) return nullptr;
  Asset* asset = GetOrCreateAsset(desc, ops);
  return LoadAsync(asset);
}

// TODO: check asset->_state
void AssetManager::Unload(Asset* asset) {
  c3_assert_return(asset);
  if (asset->_ref == 0) return;
  --asset->_ref;
  if (asset->_ref == 0 && asset->_state == ASSET_STATE_READY) {
    asset->_state = ASSET_STATE_UNLOADING;
    asset->_ops->_unload_fn(asset);
  }
}

bool AssetManager::Resolve(AssetType type, const char* filename, AssetDesc& out_desc, AssetOperations*& out_ops) {
  out_desc._type = type;
  out_desc._flags = 0;
  out_ops = &NULL_ASSET_OPS;
  strcpy(out_desc._filename, filename);

  const char* suffix = strrchr(filename, '.');
  if (!suffix) {
    c3_log("[C3] Asset resolve failed, bad filename: '%s'.", filename ? filename : "<null>");
    return false;
  }
  for (AssetLoaderType* loader = ASSET_LOADERS; loader < ASSET_LOADERS + ARRAY_SIZE(ASSET_LOADERS); ++loader) {
    if (loader->_type == type && strcmp(loader->_suffix, suffix) == 0) {
      out_ops = loader->_ops;
      break;
    }
  }

  return true;
}

Asset* AssetManager::GetOrCreateAsset(const AssetDesc& desc, AssetOperations* ops) {
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
  asset->_ops = ops;
  asset->_header = nullptr;
  asset->_ref = 1;
  return asset;
}
