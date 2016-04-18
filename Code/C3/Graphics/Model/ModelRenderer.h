#pragma once

#include "Model.h"
#include "Asset/AssetManager.h"

struct ModelPartRenderer {
  u32 _part_index;
  char _material_filename[MAX_ASSET_NAME];
};

struct ModelRenderer {
  EntityHandle _entity;
  Asset* _asset;
  void Init() {
    _asset = nullptr;
  }
};
