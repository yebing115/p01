#pragma once
#include "Pattern/Handle.h"
#include "Pattern/Singleton.h"
#include "Platform/C3Platform.h"
#include "Data/DataType.h"
#include "Data/String.h"

#define MAX_ASSET_NAME  120

enum AssetType {
  ASSET_TYPE_INVALID = -1,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MATERIAL_SHADER,
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
  char _filename[MAX_ASSET_NAME];
};
static_assert(sizeof(AssetDesc) == 128, "Bad sizeof(AssetDesc)");

struct AssetOperations {
  atomic_int* (*_load_async_fn)(Asset* asset);
  void(*_unload_fn)(Asset* asset);
};

struct AssetMemoryHeader;
struct Asset {
  atomic<AssetState> _state;
  SpinLock _lock;
  atomic_u32 _ref;
  AssetDesc _desc;
  AssetOperations* _ops;
  AssetMemoryHeader* _header;
};

#define ASSET_MEMORY_SIZE(num_depends, content_size) \
  ALIGN_MASK(ALIGN_MASK(sizeof(AssetMemoryHeader) + (num_depends) * sizeof(AssetDesc), POINTER_ALIGN_MASK) + (content_size), POINTER_SIZE)
struct AssetMemoryHeader {
  u32 _size;
  u16 _num_depends;
  AssetDesc _depends[];
  // Texture/Program/Model aligned to POINTER_SIZE
  u8* GetData() const {
    return (u8*)this + ALIGN_MASK(sizeof(AssetMemoryHeader) + _num_depends * sizeof(AssetDesc), POINTER_ALIGN_MASK);
  }
  u32 GetDataSize() const {
    return _size - (GetData() - (u8*)this);
  }
};


