#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Pattern/Handle.h"
#include "Asset/AssetManager.h"

struct ModelPart {
  u32 _start_index;
  u32 _num_indices;
  int _material_index;
  AABB _aabb;
};

struct Model {
  char _filename[MAX_ASSET_NAME];
  VertexBufferHandle _vb;
  IndexBufferHandle _ib;
  AABB _aabb;
  u16 _num_materials;
  u16 _num_parts;
  Asset** _materials;
  ModelPart* _parts;
  u8* _data[];
  static size_t ComputeSize(u16 num_materials, u16 num_parts) {
    size_t size = ALIGN_MASK(sizeof(Model), ALIGN_OF(Asset*) - 1) + num_materials * sizeof(Asset*);
    size = ALIGN_MASK(size, ALIGN_OF(ModelPart) - 1) + num_parts * sizeof(ModelPart);
    return size;
  }
  void Init(u16 num_materials, u16 num_parts) {
    _num_materials = num_materials;
    _num_parts = num_parts;
    size_t offset = ALIGN_MASK(sizeof(Model), ALIGN_OF(Asset*) - 1);
    _materials = (Asset**)((u8*)this + offset);
    offset = ALIGN_MASK(offset + num_materials * sizeof(Asset*), ALIGN_OF(ModelPart) - 1);
    _parts = (ModelPart*)((u8*)this + offset);
  }
};
