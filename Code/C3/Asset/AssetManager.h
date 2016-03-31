#pragma once
#include "Pattern/Handle.h"
#include "Pattern/Singleton.h"
#include "Platform/C3Platform.h"
#include "Data/DataType.h"
#include "Data/String.h"
#include "Graphics/Model/Model.h"
#include "Graphics/Material/Program.h"
#include "Graphics/Material/Texture.h"

enum AssetType {
  ASSET_TYPE_INVALID = -1,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_PROGRAM,
  ASSET_TYPE_MATERIAL,
  ASSET_TYPE_MODEL,
  ASSET_TYPE_COUNT,
};

enum AssetState {
  ASSET_STATE_EMPTY,
  ASSET_STATE_LOADING,
  ASSET_STATE_READY,
  ASSET_STATE_UNLOADING,
};

struct Asset;
struct AssetOperations;
struct AssetDesc {
  u32 _type;
  u32 _flags;
  char _filename[120 - POINTER_SIZE];
  AssetOperations* _ops;
};
static_assert(sizeof(AssetDesc) == 128, "Bad sizeof(AssetDesc)");

struct AssetOperations {
  void (*_load_fn)(Asset* asset);
  void (*_unload_fn)(Asset* asset);
};

struct AssetMemoryHeader;
struct Asset {
  AssetState _state;
  u32 _ref;
  AssetDesc _desc;
  AssetMemoryHeader* _header;
};

struct AssetMemoryHeader {
  u32 _size;
  u16 _num_depends;
  AssetDesc _depends[];
  // Texture/Program/Model aligned to POINTER_SIZE
  u8* GetData() const {
    return (u8*)this + ALIGN_MASK(sizeof(AssetMemoryHeader) + _num_depends * sizeof(AssetDesc), POINTER_ALIGN_MASK);
  }
};

class AssetManager {
public:
  AssetManager();
  ~AssetManager();

  Asset* Get(const AssetDesc& desc);
  Asset* Get(AssetType type, const char* filename) { return Get(Resolve(type, filename)); }
  Asset* Load(const AssetDesc& desc);
  Asset* Load(AssetType type, const char* filename) { return Load(Resolve(type, filename)); }
  void Load(Asset* asset);
  void Unload(Asset* asset);

  static AssetDesc Resolve(AssetType type, const char* filename);

private:
  unordered_map<stringid, int> _asset_map;
  Asset _assets[C3_MAX_ASSETS];
  u32 _num_assets;
  SUPPORT_SINGLETON(AssetManager);
};
