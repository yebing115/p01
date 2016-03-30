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
  Handle _vb;
  Handle _ib;
  u16 _num_parts;
  ModelPart _parts[];
};

