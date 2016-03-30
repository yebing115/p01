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
};

enum AssetState {
  ASSET_STATE_EMPTY,
  ASSET_STATE_LOADING,
};

struct AssetMemoryHeader;
struct Asset {
  AssetState _state;
  AssetMemoryHeader* _header;
};

struct AssetMemoryHeader {
  u32 _size;
  u32 _type;
  u32 _ref;
  AssetMemoryHeader* _next;
  AssetMemoryHeader* _prev;
  union {
    Texture _texture;
    Model _model;
    Program _program;
    u8 _p;
  };
};

class AssetManager {
public:
  AssetManager();
  ~AssetManager();

  Asset* Get(const String& filename) { return Get(Resolve(filename)); }
  Asset* Get(stringid filename);
  Asset* Load(const String& filename) { return Load(Resolve(filename)); }
  Asset* Load(stringid filename);
  void Load(Asset* asset);
  void Unload(Asset* asset);

private:
  static stringid Resolve(const String& filename);
  unordered_map<stringid, int> _asset_map;
  Asset _assets[C3_MAX_ASSETS];
  u32 _num_assets;
  SUPPORT_SINGLETON(AssetManager);
};
