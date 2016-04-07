#pragma once

#include "Model.h"
#include "Asset/AssetManager.h"

struct ModelPartRenderer {
  u32 _part_index;
  char _material_filename[MAX_ASSET_NAME];
};

struct ModelRenderer {
  EntityHandle _entity;
  Asset* _model;
  vector<ModelPartRenderer> _part_list;
  void Init() {
    _model = nullptr;
    _part_list.clear();
  }
};
