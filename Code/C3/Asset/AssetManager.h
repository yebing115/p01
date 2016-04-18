#pragma once
#include "Asset.h"
#include "Data/Blob.h";

class AssetManager {
public:
  AssetManager();
  ~AssetManager();

  Asset* Get(AssetType type, const char* filename);
  Asset* Load(AssetType type, const char* filename);
  atomic_int* LoadAsync(AssetType type, const char* filename);
  void Load(Asset* asset);
  atomic_int* LoadAsync(Asset* asset);
  void Unload(Asset* asset);

  void InitBuiltinAssets();
  u32 GetUsed() const { return _num_assets; }
  void Serialize(BlobWriter& writer);
  int GetAssetDenseIndex(Asset* asset) const;

  static bool Resolve(AssetType type, const char* filename, AssetDesc& out_desc, AssetOperations*& out_ops);

private:
  Asset* GetOrCreateAsset(const AssetDesc& desc, AssetOperations* ops);
  unordered_map<stringid, int> _asset_map;
  Asset _assets[C3_MAX_ASSETS];
  u32 _num_assets;
  SUPPORT_SINGLETON(AssetManager);
};
