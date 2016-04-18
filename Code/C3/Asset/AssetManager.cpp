#include "C3PCH.h"
#include "AssetManager.h"
#include "DDSTextureLoader.h"
#include "MEXModelLoader.h"
#include "MaterialLoader.h"

DEFINE_SINGLETON_INSTANCE(AssetManager);

static AssetOperations NULL_ASSET_OPS = {
  [](Asset* asset) -> atomic_int* {
    asset->_state = ASSET_STATE_EMPTY;
    return nullptr;
  },
  [](Asset* asset) {
    asset->_state = ASSET_STATE_EMPTY;
  },
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
  u32 ref = asset->_ref.fetch_sub(1);
  if (ref == 1 && asset->_state == ASSET_STATE_READY) {
    asset->_state = ASSET_STATE_UNLOADING;
    asset->_ops->_unload_fn(asset);
  }
}

struct BuiltinTexture {
  char name[MAX_ASSET_NAME];
  u32 data[4];
};

static const BuiltinTexture s_builtin_textures[] = {
  {"WHITE",{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}},
  {"RED",{0xFFFFFFFF, 0, 0, 0xFFFFFFFF}},
  {"GREEN",{0, 0, 0xFFFFFFFF, 0xFFFFFFFF}},
  {"BLUE",{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}},
  {"UP",{0x80808080, 0x80808080, 0xFFFFFFFF, 0xFFFFFFFF}},
  {"BLACK",{0, 0, 0, 0xFFFFFFFF}},
};

void AssetManager::InitBuiltinAssets() {
  auto GR = GraphicsRenderer::Instance();
  c3_assert(GR);

  for (int i = 0; i < ARRAY_SIZE(s_builtin_textures); ++i) {
    AssetDesc desc;
    strcpy(desc._filename, s_builtin_textures[i].name);
    desc._flags = 0;
    desc._type = ASSET_TYPE_TEXTURE;
    auto asset = GetOrCreateAsset(desc, &NULL_ASSET_OPS);
    asset->_state = ASSET_STATE_READY;
    auto asset_mem_size = ASSET_MEMORY_SIZE(0, sizeof(Texture));
    asset->_header = (AssetMemoryHeader*)C3_ALLOC(g_allocator, asset_mem_size);
    asset->_header->_size = asset_mem_size;
    asset->_header->_num_depends = 0;
    auto texture = (Texture*)asset->_header->GetData();
    texture->_handle = GR->CreateTexture2D(2, 2, 1, RGBA_8_TEXTURE_FORMAT, 0,
                                           mem_ref(s_builtin_textures[i].data,
                                                   sizeof(s_builtin_textures[i].data)),
                                           &texture->_info);
  }
}

void AssetManager::Serialize(BlobWriter& writer) {
  for (u32 i = 0; i < _num_assets; ++i) {
    writer.Write(_assets[i]._desc);
  }
}

int AssetManager::GetAssetDenseIndex(Asset* asset) const {
  auto asset_id = String::GetID(asset->_desc._filename);
  auto it = _asset_map.find(asset_id);
  return it == _asset_map.end() ? -1: it->second;
}

bool AssetManager::Resolve(AssetType type, const char* filename, AssetDesc& out_desc, AssetOperations*& out_ops) {
  out_desc._type = type;
  out_desc._flags = 0;
  out_ops = &NULL_ASSET_OPS;
  strcpy(out_desc._filename, filename);

  const char* suffix = strrchr(filename, '.');
  if (!suffix) { // Builtin assets.
    strcpy(out_desc._filename, filename);
    return true;
  }
  if (strcmp(suffix, ".png") == 0 || strcmp(suffix, ".tga") == 0) {
    strcpy(out_desc._filename + (suffix - filename), ".dds");
    suffix = ".dds";
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
