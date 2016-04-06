#pragma once
#include "Data/DataType.h"
#include "Data/String.h"
#include "Pattern/Handle.h"

struct ModelPart {
  u32 _start_index;
  u32 _num_indices;
};

struct Model {
  stringid _filename;
  VertexBufferHandle _vb;
  IndexBufferHandle _ib;
  u16 _num_parts;
  ModelPart _parts[];
  static size_t ComputeSize(u16 num_parts) {
    return sizeof(Model) + num_parts * sizeof(ModelPart);
  }
};

