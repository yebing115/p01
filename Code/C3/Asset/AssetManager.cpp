#include "AssetManager.h"

DEFINE_SINGLETON_INSTANCE(AssetManager);

AssetManager::AssetManager(): _num_assets(0) {}

AssetManager::~AssetManager() {

}

Asset* AssetManager::Get(stringid filename) {
  auto it = _asset_map.find(filename);
  return (it == _asset_map.end() ? nullptr : _assets + it->second);
}

Asset* AssetManager::Load(stringid filename) {
  auto it = _asset_map.find(filename);
  if (it != _asset_map.end()) return _assets + it->second;
  Asset* asset = _assets + _num_assets;
  _asset_map[filename] = _num_assets;
  ++_num_assets;
  asset->_state = ASSET_STATE_EMPTY;
  asset->_header = nullptr;
  Load(asset);
  return asset;
}

void AssetManager::Load(Asset* asset) {
}

void AssetManager::Unload(Asset* asset) {

}

stringid AssetManager::Resolve(const String& filename) {
  return filename.GetID();
}
