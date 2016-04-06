#pragma once

#include "Model.h"
#include "Asset/AssetManager.h"

struct ModelPartRenderer {
  u32 _part_index;
  char _material_filename[MAX_ASSET_NAME];
};

struct ModelRenderer {
  EntityHandle _entity;
  char _filename[MAX_ASSET_NAME];
  vector<ModelPartRenderer> _part_list;
  void Init() {
    _filename[0] = '\0';
    _part_list.clear();
  }
};
